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


//Tilde Expansion Prototypes


//$Path Search Prototypes

void execute_command(char *command, char **argv);

//External Command Execution Prototypes


//IO Redirection Prototypes


//Piping Protypes

void execute_pipeline(char ***cmds, int num_cmds);

//Background Processing Prototypes


//Internal Command Execution Prototypes


