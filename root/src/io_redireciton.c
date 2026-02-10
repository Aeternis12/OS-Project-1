#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <shell.h>

// function to handle the redirection
// call this in the child process
void handle_io_redirection(char **args) {
    char *in_file = NULL;
    char *out_file = NULL;
    int i = 0;

    // loop through args to find redirection symbols
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            // make sure there is a file after the symbol
            if (args[i+1] == NULL) {
                fprintf(stderr, "Error: No input file specified.\n");
                exit(1);
            }
            in_file = args[i+1];
        } 
        else if (strcmp(args[i], ">") == 0) {
            // make sure there is a file after the symbol
            if (args[i+1] == NULL) {
                fprintf(stderr, "Error: No output file specified.\n");
                exit(1);
            }
            out_file = args[i+1];
        }
        i++;
    }

    // handle input redirection if we found it
    if (in_file != NULL) {
        struct stat sb;
        
        // check if file exists
        if (stat(in_file, &sb) == -1) {
            fprintf(stderr, "Error: Input file does not exist.\n");
            exit(1);
        }
        
        // make sure its a regular file
        if (!S_ISREG(sb.st_mode)) {
            fprintf(stderr, "Error: Input file is not a regular file.\n");
            exit(1);
        }

        // open the file for reading
        int fd0 = open(in_file, O_RDONLY);
        if (fd0 == -1) {
            perror("Error opening input file");
            exit(1);
        }

        // swap stdin with our file
        if (dup2(fd0, STDIN_FILENO) == -1) {
            perror("dup2 input failed");
            exit(1);
        }
        close(fd0);
    }

    // handle output redirection if we found it
    if (out_file != NULL) {
        // open file, create if needed, truncate if exists
        // permissions set to read/write for user only (required by instructions)
        int fd1 = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        
        if (fd1 == -1) {
            perror("Error opening output file");
            exit(1);
        }

        // swap stdout with our file
        if (dup2(fd1, STDOUT_FILENO) == -1) {
            perror("dup2 output failed");
            exit(1);
        }
        close(fd1);
    }

    // clean up the args array
    // remove the symbols and filenames so execv doesn't get confused
    int j = 0;
    i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0) {
            // skip the symbol and the filename
            i += 2;
        } else {
            // keep this arg
            args[j] = args[i];
            i++;
            j++;
        }
    }
    args[j] = NULL; // make sure to null terminate
}

int main() {
    printf("--- Part 6 Test ---\n");
    printf("Command: ls -l > test_output.txt\n");

    // hardcoded args to simulate the parser output
    // using /bin/ls directly so we don't need path search
    char *args[] = { "/bin/ls", "-l", ">", "test_output.txt", NULL };

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // child process
        
        // do the redirection logic
        handle_io_redirection(args);

        // run the command
        execv(args[0], args);
        
        // if we get here, execv failed
        perror("execv failed");
        exit(1);
    } 
    else {
        // parent process
        wait(NULL);
        printf("--- Finished ---\n");
        printf("Check test_output.txt to see if it worked.\n");
    }

    return 0;
}
