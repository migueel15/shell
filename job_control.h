/*--------------------------------------------------------
UNIX Shell Project
function prototypes, macros and type declarations for job_control module

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.
--------------------------------------------------------*/

#ifndef _JOB_CONTROL_H
#define _JOB_CONTROL_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

// ----------- ENUMERATIONS ---------------------------------------------
enum status { SUSPENDED, SIGNALED, EXITED, CONTINUED };
enum job_state { FOREGROUND, BACKGROUND, STOPPED, RESPAWNABLE };
static const char *status_strings[] = {"Suspended", "Signaled", "Exited",
                                       "Continued"};
static const char *state_strings[] = {"Foreground", "Background", "Stopped",
                                      "Respawnable"};

// ----------- JOB TYPE FOR JOB LIST ------------------------------------
typedef struct job_ {
  pid_t pgid;    /* group id = process lider id */
  char *command; /* program name */
  enum job_state state;
  struct job_ *next; /* next job in the list */
  char **args;
} job;

// -----------------------------------------------------------------------
//      PUBLIC FUNCTIONS
// -----------------------------------------------------------------------
void get_command(char inputBuffer[], int size, char *args[], int *background,
                 int *respawnable);
job *new_job(pid_t pid, const char *command, char *args[],
             enum job_state state);
void add_job(job *list, job *item);
int delete_job(job *list, job *item);
job *get_item_bypid(job *list, pid_t pid);
job *get_item_bypos(job *list, int n);
enum status analyze_status(int status, int *info);

// -----------------------------------------------------------------------
//      PRIVATE FUNCTIONS PROTOTYPES: BETTER USED THROUGH MACROS BELOW
// -----------------------------------------------------------------------
void print_item(job *item);
void print_list(job *list, void (*print)(job *));
void terminal_signals(void (*func)(int));
void block_signal(int signal, int block);
void mask_signal(int signal, int block);

// -----------------------------------------------------------------------
//      PUBLIC MACROS
// -----------------------------------------------------------------------
#define list_size(list) (list->pgid) // number of jobs in the list
#define empty_list(list)                                                       \
  (!(list->pgid)) // returns 1 (true) if the list is empty

#define new_list(name)                                                         \
  new_job(0, name, NULL, FOREGROUND) // name must be const char *

#define print_job_list(list) print_list(list, print_item)

#define restore_terminal_signals() terminal_signals(SIG_DFL)
#define ignore_terminal_signals() terminal_signals(SIG_IGN)

#define new_process_group(pid) setpgid(pid, pid)

#define block_SIGCHLD() mask_signal(SIGCHLD, SIG_BLOCK)
#define unblock_SIGCHLD() mask_signal(SIGCHLD, SIG_UNBLOCK)

// macro for debugging----------------------------------------------------
// to debug integer i, use:    debug(i,%d);
// it will print out:  current line number, function name and file name, and
// also variable name, value and type
#define debug(x, fmt)                                                          \
  fprintf(stderr, "\"%s\":%u:%s(): --> %s= " #fmt " (%s)\n", __FILE__,         \
          __LINE__, __FUNCTION__, #x, x, #fmt)

// -----------------------------------------------------------------------
#endif
