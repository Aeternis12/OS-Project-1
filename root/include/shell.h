//header file

#ifndef SHELL_H
#define SHELL_H


//CONSTANTS
#define MAX_JOBS 10
#define HISTORY_DEPTH 3




//DATA STRUCTS
struct Job {
    int id;
    int pid;
    char* command;
    int is_active;
};

//Global Variables
extern struct Job jobs[MAX_JOBS];
extern char* history[HISTORY_DEPTH];
extern int history_count;


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

void execute_command(char *command, char **argv);

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
void builtin_jobs(void);
int builtin_cd(char **args);


#endif // SHELL_H


