#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

// function to handle the redirection
// call this in the child process
void handle_io_redirection(char **args) {
    char *in_file = NULL;
    char *out_file = NULL;
    int i = 0;

    // loop through args to find redirection symbols
    while (args && args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Error: No input file specified.\n");
                exit(1);
            }
            in_file = args[i+1];
        } 
        else if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Error: No output file specified.\n");
                exit(1);
            }
            out_file = args[i+1];
        }
        i++;
    }

    if (in_file != NULL) {
        struct stat sb;
        if (stat(in_file, &sb) == -1) {
            fprintf(stderr, "Error: Input file does not exist.\n");
            exit(1);
        }
        if (!S_ISREG(sb.st_mode)) {
            fprintf(stderr, "Error: Input file is not a regular file.\n");
            exit(1);
        }

        int fd0 = open(in_file, O_RDONLY);
        if (fd0 == -1) {
            perror("Error opening input file");
            exit(1);
        }
        if (dup2(fd0, STDIN_FILENO) == -1) {
            perror("dup2 input failed");
            exit(1);
        }
        close(fd0);
    }

    if (out_file != NULL) {
        int fd1 = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd1 == -1) {
            perror("Error opening output file");
            exit(1);
        }
        if (dup2(fd1, STDOUT_FILENO) == -1) {
            perror("dup2 output failed");
            exit(1);
        }
        close(fd1);
    }

    if (!args) return;
    int j = 0;
    i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
            i += 2;
        } else {
            args[j++] = args[i++];
        }
    }
    args[j] = NULL;
}

//int main() {
//    printf("--- Part 6 Test ---\n");
//    printf("Command: ls -l > test_output.txt\n");
//
//    // hardcoded args to simulate the parser output
//    // using /bin/ls directly so we don't need path search
//    char *args[] = { "/bin/ls", "-l", ">", "test_output.txt", NULL };
//
//    pid_t pid = fork();
//
//    if (pid == -1) {
//        perror("fork failed");
//        return 1;
//    } 
//    else if (pid == 0) {
//        // child process
//        
//        // do the redirection logic
//        handle_io_redirection(args);
//
//        // run the command
//        execv(args[0], args);
//        
//        // if we get here, execv failed
//        perror("execv failed");
//        exit(1);
//    } 
//    else {
//        // parent process
//        wait(NULL);
//        printf("--- Finished ---\n");
//        printf("Check test_output.txt to see if it worked.\n");
//    }
//
//    return 0;
//}
