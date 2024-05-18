#include "builtin_commands.h"
#include "job_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

s_command builtin_commands[] = {{EXIT, "exit"}, {CD, "cd"},
                                {JOBS, "jobs"}, {FG, "fg"},
                                {BG, "bg"},     {ALARM_THREAD, "alarm-thread"}};

e_builtin check_if_builtin(char *command) {
  // int maxLen = sizeof(builtin_commands) / sizeof(s_command);
  int maxLen = 6;
  for (int i = 0; i < maxLen; i++) {
    if (strcmp(command, builtin_commands[i].commandString) == 0) {
      return builtin_commands[i].commandEnum;
    }
  }
  return -1;
}

void run_builtin_command(e_builtin COMMAND, char *args[], job *job_list,
                         s_alarm_thread_args *alarm_thread_args) {
  switch (COMMAND) {
  case EXIT:
    exit(0);
    break;
  case CD:
    change_directory(args);
    break;
  case JOBS:
    show_jobs(job_list);
    break;
  case FG:
    send_fg(args, job_list);
    break;
  case BG:
    send_bg(args, job_list);
    break;
  case ALARM_THREAD:
    alarm_thread(args, job_list, alarm_thread_args);
    break;
  }
}

void change_directory(char *args[]) {
  if (args[1] == NULL) {
    chdir(getenv("HOME"));
    return;
  }
  if (chdir(args[1]) != 0) {
    perror("cd");
  };
}

void show_jobs(job *job_list) {
  block_SIGCHLD();
  print_job_list(job_list);
  unblock_SIGCHLD();
}

void send_fg(char *args[], job *job_list) {
  block_SIGCHLD();
  int status;
  int info;
  enum status;
  int posicion = 1;
  if (args[1] != NULL) {
    posicion = atoi(args[1]);
  }
  job *item = get_item_bypos(job_list, posicion);
  if (item == NULL) {
    return;
  }
  tcsetpgrp(STDIN_FILENO, item->pgid);     // damos la terminal al proceso
  killpg(item->pgid, SIGCONT);             // mando señal para que continue
  waitpid(item->pgid, &status, WUNTRACED); // espero a que cambie de estado
  tcsetpgrp(STDIN_FILENO, getpid());       // devuelvo la terminal al padre
  status = analyze_status(status, &info);
  printf("Foreground pid: %d, command: %s, %s, info: %d\n", item->pgid, args[0],
         status_strings[status], info);
  if (status == EXITED || status == SIGNALED) {
    delete_job(job_list, item);
  } else if (status == SUSPENDED) {
    item->state = STOPPED;
  }

  unblock_SIGCHLD();
}
void send_bg(char *args[], job *job_list) {
  block_SIGCHLD();

  int posicion = 1;
  if (args[1] != NULL) {
    posicion = atoi(args[1]);
  }
  job *item = get_item_bypos(job_list, posicion);
  if (item == NULL) {
    return;
  }
  if (item->state == STOPPED || item->state == RESPAWNABLE) {
    item->state = BACKGROUND;
    killpg(item->pgid, SIGCONT);
  }
  unblock_SIGCHLD();
}

void *sleepTimeout(void *args) {
  s_alarm_thread_args *alarm_thread_args = (s_alarm_thread_args *)args;
  sleep(alarm_thread_args->seconds_to_sleep);
  kill(alarm_thread_args->pid, SIGKILL);
  free(alarm_thread_args);
  return NULL;
}

void alarm_thread(char **args, job *job_list,
                  s_alarm_thread_args *alarm_thread_args) {
  if (args[1] == NULL) {
    perror("Usage: alarm-thread <seconds>\n");
    return;
  }
  int seconds = atoi(args[1]);
  if (seconds <= 0) {
    perror("Usage: alarm-thread <seconds>\n");
    return;
  }

  alarm_thread_args->active = 1;
  alarm_thread_args->seconds_to_sleep = seconds;

  // shift args 2 positions to the left
  int i = 0;
  args[0] = NULL;
  args[1] = NULL;
  while (args[i + 2] != NULL) {
    args[i] = strdup(args[i + 2]);
    args[i + 2] = NULL;
    i++;
  }
}
