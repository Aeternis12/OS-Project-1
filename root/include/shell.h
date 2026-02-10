//header file

#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lexer.h"

//CONSTANTS
#define MAX_ACTIVE_JOBS 10
#define MAX_JOB_HISTORY 1024 /* capacity for job records (job numbers monotonic) */
#define MAX_PROCS_PER_JOB 3  /* supports up to 3 commands per job (2 pipes => 3 procs) */
#define HISTORY_DEPTH 3




//DATA STRUCTS
typedef struct job {
    int active;                 /* 1 if active, 0 if finished */
    int jobno;                  /* monotonic job number */
    pid_t pids[MAX_PROCS_PER_JOB];
    int nprocs;                 /* number of pids stored */
    pid_t leader_pid;           /* pid printed at start (last pid in pipeline) */
    int remaining;              /* how many procs still running */
    char* cmdline;              /* strdup'd command line for messages */
} job_t;

//Global Variables
extern job_t job_table[]; // Defined in background_proc.c
extern int next_job_index;


//------------Function Prototypes----------------\\


//Tokenization Prototypes


//Prompt Prototypes

void print_prompt(void);

//Environment Variable Prototypes

void free_argv(char **argv);
char **expand_env_vars_dup(char **argv);
int expand_env_vars_inplace(char **argv);

//Tilde Expansion Prototypes

char* expand_tilde(char *token);


//$Path Search Prototypes

void execute_search(char *command, char **argv);

//External Command Execution Prototypes

char *find_executable(const char *cmd);
pid_t execute_command(char **argv, char *fullpath, int background);

//IO Redirection Prototypes

void handle_io_redirection(char **args);


//Piping Protypes

void execute_pipeline(char ***cmds, int num_cmds);

//Background Processing Prototypes

void part_eight_init(void);
int part_eight_add_job(const char *cmdline, pid_t *pids, int nprocs, pid_t leader_pid);
void part_eight_check_jobs(void);
void part_eight_jobs_builtin(void);
void part_eight_shutdown(void);

//Internal Command Execution Prototypes

void add_to_history(char *cmd);
void builtin_exit(void);
int builtin_cd(char **args);


#endif // SHELL_H


