#include "builtin_commands.h"
#include "job_control.h" // remember to compile with module job_control.c
#include "parse_redir.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE                                                               \
  256 /*erto 256 chars per line, per command, should be enough. */
job *job_list;

// return 1 if error
int change_inout(char *file_in, char *file_out) {
  int in_fd, out_fd;

  if (file_in != NULL) {
    in_fd = open(file_in, O_RDONLY); // read only
    if (in_fd == -1) {
      perror("Error opening input file");
      return 1;
    }
    dup2(in_fd, STDIN_FILENO);
    close(in_fd);
  }

  if (file_out != NULL) {
    // write only, create if not exists, delete contenct, if exists,
    // privileges
    out_fd = open(file_out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
      perror("Error opening output file");
      return 1;
    }
    dup2(out_fd, STDOUT_FILENO);
    close(out_fd);
  }

  return 0;
}

void manejador(int sig) {
  mask_signal(SIGCHLD, SIG_BLOCK);
  int pid_wait;
  int status;
  int info;
  enum status status_analyzed;
  job *current;

  while ((pid_wait = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) >
         0) {
    current = get_item_bypid(job_list, pid_wait);
    status_analyzed = analyze_status(status, &info);

    if (status_analyzed == SUSPENDED) {
      printf("Background pid: %d, command: %s, Suspended, info: %d\n", pid_wait,
             current->command, info);
      current->state = STOPPED;
    } else if (status_analyzed == CONTINUED) {
      printf("Background pid: %d, command: %s, Continued, info: %d\n", pid_wait,
             current->command, info);
      current->state = BACKGROUND;
    } else if (status_analyzed == SIGNALED || status_analyzed == EXITED) {
      if (current->state == RESPAWNABLE) {
        printf(
            "Background Respawneable pid: %d, command: %s, Exited, info: %d\n",
            pid_wait, current->command, info);
        current->state = RESPAWNABLE;

        int pid_fork = fork();
        if (pid_fork > 0) {
          current->pgid = pid_fork;
        } else {
          execvp(current->command, current->args);
          perror("Error al ejecutar el comando");
          exit(-1);
        }

      } else {
        printf("Background pid: %d, command: %s, Exited, info: %d\n", pid_wait,
               current->command, info);
        delete_job(job_list, current);
      }
    }
  }
  mask_signal(SIGCHLD, SIG_UNBLOCK);
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------
//

int main(void) {
  char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
  int background;             /* equals 1 if a command is followed by '&' */
  int respawneable;
  int internal_command;
  char *args[MAX_LINE / 2]; /* command line (of 256) has max of 128 arguments */
  // probably useful variables:
  int pid_fork, pid_wait; /* pid for created and waited process */
  int status;             /* status returned by wait */
  enum status status_res; /* status processed by analyze_status() */
  int info;               /* info processed by analyze_status() */

  char *file_in;
  char *file_out;

  // store the original descriptors to restore them later
  int original_stdin = dup(STDIN_FILENO);
  int original_stdout = dup(STDOUT_FILENO);

  job_list = new_list("Lista de procesos");
  terminal_signals(SIG_IGN);
  signal(SIGCHLD, manejador);

  while (1) {
    // reset redirections
    file_in = NULL;
    file_out = NULL;
    // restore original descriptors
    dup2(original_stdin, STDIN_FILENO);
    dup2(original_stdout, STDOUT_FILENO);

    printf("COMMAND->");
    fflush(stdout);
    get_command(inputBuffer, MAX_LINE, args, &background, &respawneable);

    if (args[0] == NULL) {
      continue;
    }

    parse_redirections(args, &file_in, &file_out);
    int err = change_inout(file_in, file_out);
    if (err) {
      continue;
    }

    // -------- BUILTIN COMMANDS -------- //
    // returns -1 if not a builtin command
    e_Builtin COMMAND = check_if_builtin(args[0]);
    if (COMMAND != -1) {
      run_builtin_command(COMMAND, args, job_list);
      continue;
    }
    // --------------------------------- //

    pid_fork = fork();
    if (pid_fork == -1) {
      perror("Error al crear el proceso hijo");
      continue;
    }

    if (pid_fork == 0) {
      // hijo
      setpgid(getpid(), getpid());
      if (background == 0) {
        tcsetpgrp(STDIN_FILENO, getpid());
      }
      terminal_signals(SIG_DFL);
      execvp(args[0], args);
      perror("Error al ejecutar el comando");
      exit(-1);
    } else {
      // padre
      if (background == 0) {
        // foreground
        pid_wait = waitpid(pid_fork, &status, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        status_res = analyze_status(status, &info);
        if (status_res == SUSPENDED) {
          mask_signal(SIGCHLD, SIG_BLOCK);
          job *newjob = new_job(pid_fork, args[0], args, STOPPED);
          add_job(job_list, newjob);
          printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait,
                 args[0], status_strings[status_res], info);
          mask_signal(SIGCHLD, SIG_UNBLOCK);
        } else if (status_res == EXITED || status_res == SIGNALED) {

          printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait,
                 args[0], status_strings[status_res], info);
        }
      } else {
        // background
        mask_signal(SIGCHLD, SIG_BLOCK);
        job *newjob = NULL;
        if (respawneable == 1) {
          newjob = new_job(pid_fork, args[0], args, RESPAWNABLE);
          printf("Background Respawnable job runing... pid: %d, command: %s\n",
                 pid_fork, args[0]);
        } else {
          newjob = new_job(pid_fork, args[0], args, BACKGROUND);
          printf("Background job runing... pid: %d, command: %s\n", pid_fork,
                 args[0]);
        }
        add_job(job_list, newjob);
        mask_signal(SIGCHLD, SIG_UNBLOCK);
      }
    }
  }
}
