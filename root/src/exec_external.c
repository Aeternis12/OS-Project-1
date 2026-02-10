//**********************************************************************************************************
//* Name:        exec_external.c                                                                           *
//* Description: External-command execution utilities for COP4610 Project 1 shell.                         *
//*              Implements:                                                                               *
//*                - find_executable(cmd): locate an executable via PATH or accept                         *
//*                  a path containing '/' (returns malloc'd string or NULL).                              *
//*                - execute_command(argv, fullpath, background): fork + execv the                         *
//*                  command; supports foreground and background execution.                                *
//*                                                                                                        *
//*              Expected integration:                                                                     *
//*                - Call after tokenization and expansion (Parts 2 & 3).                                  *
//*                - Perform redirection / pipe FD setup in the child (after fork)                         *
//*                  and before calling execute_command (or have execute_command                           *
//*                  called after those setups are done).                                                  *
//*                - Built-ins (cd/exit/jobs) should be handled before calling                             *
//*                  execute_command.                                                                      *
//*                                                                                                        *
//* Author:      Katelyna Pastrana                                                                         *
//* Date:        2026-02-03                                                                                *
//* References:                                                                                            *
//*    - COP4610 Project 1 specification (Part 5: External Command Execution)                              *
//*    - POSIX: fork(2), execv(2), waitpid(2), access(2), stat(2)                                          *
//*    - Stephen Brennan, "Write a Shell in C" (design notes)                                              *
//* Compile:     gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -o exec_external exec_external.c *
//**********************************************************************************************************

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include "shell.h"

/* Finds an executable on PATH.
 * If cmd contains a '/', returns strdup(cmd) (no PATH search).
 * Otherwise searches directories in getenv("PATH") and returns strdup(fullpath)
 * for the first entry where access(fullpath, X_OK) == 0.
 * Returns NULL if not found. Caller must free() returned string.
 */
char *find_executable(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return NULL;

    if (strchr(cmd, '/')) {
        return strdup(cmd);
    }

    const char *path_env = getenv("PATH");
    if (!path_env) path_env = "/bin:/usr/bin";

    char *path_dup = strdup(path_env);
    if (!path_dup) return NULL;

    char *saveptr = NULL;
    char *dir = strtok_r(path_dup, ":", &saveptr);
    while (dir) {
        size_t len = strlen(dir) + 1 + strlen(cmd) + 1;
        char *candidate = malloc(len);
        if (!candidate) {
            free(path_dup);
            return NULL;
        }
        snprintf(candidate, len, "%s/%s", dir, cmd);

        if (access(candidate, X_OK) == 0) {
            free(path_dup);
            return candidate; /* caller must free */
        }

        free(candidate);
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_dup);
    return NULL;
}

/* Print a helpful error for exec failures or missing executable */
static void print_exec_error(const char *prog, const char *path) {
    if (!path) {
        fprintf(stderr, "%s: command not found\n", prog);
        return;
    }
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "%s: %s: %s\n", prog, path, strerror(errno));
    } else {
        if (! (st.st_mode & S_IXUSR)) {
            fprintf(stderr, "%s: %s: Permission denied\n", prog, path);
        } else {
            fprintf(stderr, "%s: %s: Cannot execute\n", prog, path);
        }
    }
}

/* Execute external command.
 * argv: NULL-terminated array of arguments (argv[0] is command name)
 * fullpath: full path to executable (may be NULL). If NULL, function will attempt
 *           to find the executable via PATH. If non-NULL, it will be used as-is.
 * background: non-zero => run in background (parent does not wait).
 *
 * Returns:
 *  - child's PID (> 0) on success (parent side)
 *  - -1 on error (e.g., fork failure or executable not found)
 *
 * Notes:
 *  - If fullpath is NULL and find_executable() allocates a path, this function
 *    frees it before returning. If you pass a malloc'd fullpath yourself and want
 *    it preserved, pass it and manage freeing yourself.
 *  - Caller must perform redirection/piping FD setup in the child (after fork)
 *    before execv; alternatively, perform setups here before execv if desired.
 */
pid_t execute_command(char **argv, char *fullpath, int background) {
    if (!argv || !argv[0]) {
        errno = EINVAL;
        return -1;
    }

    int should_free_path = 0;
    char *path_to_exec = fullpath;

    if (!path_to_exec) {
        path_to_exec = find_executable(argv[0]);
        should_free_path = (path_to_exec != NULL);
    }

    if (!path_to_exec) {
        print_exec_error(argv[0], NULL);
        return -1;
    }

    if (access(path_to_exec, X_OK) != 0) {
        print_exec_error(argv[0], path_to_exec);
        if (should_free_path) free(path_to_exec);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        if (should_free_path) free(path_to_exec);
        return -1;
    }

    if (pid == 0) {
        /* Child process: restore default signal handlers so program responds
         * to signals like interactive programs normally do.
         */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

		handle_io_redirection((char**)argv);

        /* Execute program. execv only, per project restrictions. */
        execv(path_to_exec, (char *const *)argv);

        /* If execv returns, an error occurred. Print and exit child. */
        fprintf(stderr, "%s: failed to execute %s: %s\n", argv[0], path_to_exec, strerror(errno));
        _exit(127);
    }

    /* Parent */
    if (should_free_path) free(path_to_exec);

    if (background) {
        /* For background jobs, parent should not wait here. Caller must record
         * job number/pid and print job-start info. */
        return pid;
    }

    /* Foreground: wait for child and reap status */
    int status;
    pid_t w = waitpid(pid, &status, 0);
    if (w == -1) {
        perror("waitpid");
        return pid;
    }

    /* Optionally use WIFEXITED / WEXITSTATUS / WIFSIGNALED here to update
     * shell state (e.g., last exit code). */
    (void)status;

    return pid;
}
