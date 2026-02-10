#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "shell.h"

// --- GLOBALS AND DEFINITIONS ---
char* history[HISTORY_DEPTH];
int history_count = 0;

// helper to add a command to history
// call this every time a user enters a valid command
void add_to_history(char *cmd) {
    if (!cmd) return;
    // if history is full, remove the oldest one
    if (history_count == HISTORY_DEPTH) {
        free(history[0]);
        // shift everything down
        for (int i = 0; i < HISTORY_DEPTH - 1; i++) {
            history[i] = history[i+1];
        }
        history_count--;
    }
    
    // add the new one
    history[history_count] = strdup(cmd);
    history_count++;
}

// --- BUILT-IN FUNCTIONS ---

// 1. EXIT COMMAND
void builtin_exit(void) {
    printf("[DEBUG] Exiting shell...\n");

    /* Use the background-job API to shut down/cleanup instead of touching job internals. */
    part_eight_shutdown();

    // print history
    if (history_count == 0) {
        printf("No valid commands in history.\n");
    } else {
        printf("Last commands:\n");
        for (int i = 0; i < history_count; i++) {
            printf("%d: %s\n", i + 1, history[i]);
            free(history[i]); // clean up memory
        }
    }

    exit(0);
}

// 3. CD COMMAND
// returns 1 if successful, 0 if error (so we know if we should save it to history)
int builtin_cd(char **args) {
    char *target;

    // case 1: "cd" -> go home
    if (args[1] == NULL) {
        target = getenv("HOME");
        if (!target) {
            fprintf(stderr, "Error: HOME not set.\n");
            return 0;
        }
    } 
    // case 2: "cd dir" -> go to dir
    else if (args[2] == NULL) {
        target = args[1];
    } 
    // case 3: "cd dir1 dir2" -> error
    else {
        fprintf(stderr, "Error: Too many arguments.\n");
        return 0;
    }

    // try to change directory
    if (chdir(target) != 0) {
        perror("Error changing directory");
        return 0;
    }
    
    // update PWD env variable just to be safe
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("PWD", cwd, 1);
    }
    return 1;
}

// helper to parse input (simplified for this test)
// splits string by spaces
//void parse_input(char *line, char **args) {
//    int i = 0;
//    char *token = strtok(line, " \n");
//    while (token != NULL) {
//        args[i++] = token;
//        token = strtok(NULL, " \n");
//    }
//    args[i] = NULL;
//}

//int main() {
//    printf("--- Part 9 Tester (Interactive) ---\n");
//    printf("Commands to try:\n");
//    printf("  1. 'cd ..' (check if directory changes)\n");
//    printf("  2. 'jobs' (will be empty initially)\n");
//    printf("  3. 'exit' (will show history)\n");
//    printf("  4. Any other word (will be added to history)\n\n");
//
//    // Adding a fake job just so you can see 'jobs' print something
//    jobs[0].id = 1;
//    jobs[0].pid = 12345;
//    jobs[0].command = "sleep 100";
//    jobs[0].is_active = 1; 
//    printf("[Setup] Added fake background job: [1]+ [12345] sleep 100\n\n");
//
//    char line[1024];
//    char *args[100];
//    char cwd[1024];
//
//    while (1) {
//        // print prompt with current dir
//        if (getcwd(cwd, sizeof(cwd)) != NULL) {
//            printf("%s> ", cwd);
//        } else {
//            printf("shell> ");
//        }
//
//        if (fgets(line, sizeof(line), stdin) == NULL) break;
//        
//        // ignore empty lines
//        if (strcmp(line, "\n") == 0) continue;
//
//        // make a copy for history before strtok messes it up
//        // remove newline char
//        char *line_copy = strdup(line);
//        line_copy[strcspn(line_copy, "\n")] = 0;
//
//        parse_input(line, args);
//
//        if (args[0] == NULL) continue;
//
//        // check built-ins
//        if (strcmp(args[0], "exit") == 0) {
//            add_to_history(line_copy);
//            builtin_exit(); // this will exit the program
//        } 
//        else if (strcmp(args[0], "jobs") == 0) {
//            add_to_history(line_copy);
//            builtin_jobs();
//        } 
//        else if (strcmp(args[0], "cd") == 0) {
//            if (builtin_cd(args)) {
//                add_to_history(line_copy);
//            }
//        } 
//        else {
//            // treat anything else as a "command" for history testing
//            printf("Command '%s' not implemented in tester, but added to history.\n", args[0]);
//            add_to_history(line_copy);
//        }
//        
//        free(line_copy);
//    }
//    return 0;
//}
