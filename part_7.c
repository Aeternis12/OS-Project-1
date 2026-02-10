#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * cmds: array of commands
 *   cmds[i] is a NULL-terminated argv array
 *   cmds[i][0] is the command name
 *
 * num_cmds: 2 or 3
 */
void execute_command(char *command, char **argv);

/*
 * Execute:
 *   cmd1 | cmd2
 *   cmd1 | cmd2 | cmd3
 */
void execute_pipeline(char ***cmds, int num_cmds) 
{
    int pipe1[2], pipe2[2];
    pid_t pid;

    if (num_cmds >= 2) 
    {
        if (pipe(pipe1) == -1) 
        {
            perror("pipe");
            return;
        }
    }

    if (num_cmds == 3) 
    {
        if (pipe(pipe2) == -1) 
        {
            perror("pipe");
            return;
        }
    }

    // CMD 1
    pid = fork();
    if (pid == 0) 
    {
        if (num_cmds >= 2) 
        {
            dup2(pipe1[1], STDOUT_FILENO);
        }

        close(pipe1[0]);
        close(pipe1[1]);
        if (num_cmds == 3) 
        {
            close(pipe2[0]);
            close(pipe2[1]);
        }

        execute_command(cmds[0][0], cmds[0]);
        exit(1);
    }

    // CMD 2
    if (num_cmds >= 2) 
    {
        pid = fork();
        if (pid == 0) 
        {
            dup2(pipe1[0], STDIN_FILENO);

            if (num_cmds == 3) 
            {
                dup2(pipe2[1], STDOUT_FILENO);
            }

            close(pipe1[0]);
            close(pipe1[1]);
            if (num_cmds == 3) 
            {
                close(pipe2[0]);
                close(pipe2[1]);
            }

            execute_command(cmds[1][0], cmds[1]);
            exit(1);
        }
    }

    // CMD 3
    if (num_cmds == 3) 
    {
        pid = fork();
        if (pid == 0)
        {
            dup2(pipe2[0], STDIN_FILENO);

            close(pipe2[0]);
            close(pipe2[1]);
            close(pipe1[0]);
            close(pipe1[1]);

            execute_command(cmds[2][0], cmds[2]);
            exit(1);
        }
    }

    //parent

    if (num_cmds >= 2) 
    {
        close(pipe1[0]);
        close(pipe1[1]);
    }
    if (num_cmds == 3) 
    {
        close(pipe2[0]);
        close(pipe2[1]);
    }

    // wait for all children
    for (int i = 0; i < num_cmds; i++) 
    {
        wait(NULL);
    }
}