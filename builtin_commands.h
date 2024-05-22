#ifndef BUILTIN_COMMANDS_H
#define BUILTIN_COMMANDS_H

#include "job_control.h"

typedef enum {
  CD,
  EXIT,
  JOBS,
  FG,
  BG,
  ALARM_THREAD,
  DELAY_THREAD,
  MASK,
} e_builtin;

typedef struct {
  e_builtin commandEnum;
  const char *commandString;
} s_command;

typedef struct {
  int active;
  int seconds_to_sleep;
  int pid;
} s_alarm_thread_args;

/*
 * Devuelve el enum correspondiente al comando pasado por parametro si se
 * reconoce como builtin. En otro caso devuelve -1.
 */
e_builtin check_if_builtin(char *command);
void run_builtin_command(e_builtin COMMAND, char *args[], job *jobs,
                         s_alarm_thread_args *alarm_thread_args);
void change_directory(char *args[]);
void show_jobs(job *job_list);
void send_fg(char *args[], job *job_list);
void send_bg(char *args[], job *job_list);
void alarm_thread(char *args[], job *job_list, s_alarm_thread_args *ptr);
void *sleepTimeoutKill(void *args);
void *delay_thread(void *params);
void mask(char *args[]);

#endif
