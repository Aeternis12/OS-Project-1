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


//Environment Variable Prototypes


//Tilde Expansion Prototypes


//$Path Search Prototypes


//External Command Execution Prototypes


//IO Redirection Prototypes


//Piping Protypes


//Background Processing Prototypes


//Internal Command Execution Prototypes


