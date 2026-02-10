#include "shell.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Helper: join argv into a single command line for history/job messages.
 * Caller must free returned string.
 */
static char *join_argv(char **argv) {
    if (!argv || !argv[0]) return NULL;
    size_t len = 0;
    for (int i = 0; argv[i]; ++i) {
        len += strlen(argv[i]) + 1; /* space or nul */
    }
    char *out = malloc(len + 1);
    if (!out) return NULL;
    out[0] = '\0';
    for (int i = 0; argv[i]; ++i) {
        if (i) strcat(out, " ");
        strcat(out, argv[i]);
    }
    return out;
}

/* Convert tokenlist -> NULL-terminated argv (shallow copy of pointers).
 * Caller must free returned array (not strings).
 */
static char **tokenlist_to_argv(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return NULL;
    char **argv = calloc(tokens->size + 1, sizeof(char*));
    if (!argv) return NULL;
    for (size_t i = 0; i < tokens->size; ++i) argv[i] = tokens->items[i];
    argv[tokens->size] = NULL;
    return argv;
}

/* Process one command (tokenized). Handles tilde/env expansion, builtins,
 * background marker &, pipelines and external execution.
 */
static void process_command(tokenlist *tokens) {
    if (!tokens || tokens->size == 0) return;

    /* quick detect pipeline */
    int pipe_count = 0;
    for (size_t i = 0; i < tokens->size; ++i) {
        if (strcmp(tokens->items[i], "|") == 0) pipe_count++;
    }

    /* If pipeline present, build cmds and call execute_pipeline */
    if (pipe_count > 0) {
        int num_cmds = pipe_count + 1;
        char ***cmds = calloc(num_cmds, sizeof(char**));
        if (!cmds) return;

        size_t start = 0;
        int cmd_index = 0;
        for (size_t i = 0; i <= tokens->size; ++i) {
            if (i == tokens->size || strcmp(tokens->items[i], "|") == 0) {
                size_t count = i - start;
                cmds[cmd_index] = calloc(count + 1, sizeof(char*));
                if (!cmds[cmd_index]) { /* cleanup on failure */ 
                    for (int k = 0; k < cmd_index; ++k) free(cmds[k]);
                    free(cmds);
                    return;
                }
                for (size_t j = 0; j < count; ++j) cmds[cmd_index][j] = tokens->items[start + j];
                cmds[cmd_index][count] = NULL;
                cmd_index++;
                start = i + 1;
            }
        }
        execute_pipeline(cmds, num_cmds);
        for (int i = 0; i < num_cmds; ++i) free(cmds[i]);
        free(cmds);
        return;
    }

    /* Non-pipeline: shallow argv + create a mutable strdup array for expansions */
    char **argv = tokenlist_to_argv(tokens);
    if (!argv || !argv[0]) { free(argv); return; }

    /* detect background marker '&' as last token */
    int background = 0;
    int argc = 0;
    while (argv[argc]) argc++;
    if (argc > 0 && strcmp(argv[argc-1], "&") == 0) {
        background = 1;
        argv[argc-1] = NULL;
        argc--;
    }

    /* Create dup_argv with strdup for safe modification/freeing */
    char **dup_argv = calloc(argc + 1, sizeof(char*));
    if (!dup_argv) { free(argv); return; }
    for (int i = 0; i < argc; ++i) dup_argv[i] = strdup(argv[i]);
    dup_argv[argc] = NULL;

    /* Tilde expansion */
    for (int i = 0; dup_argv[i]; ++i) {
        char *expanded = expand_tilde(dup_argv[i]);
        if (expanded != dup_argv[i]) { /* expand_tilde returns strdup when expanded */
            free(dup_argv[i]);
            dup_argv[i] = expanded;
        }
    }

    /* Environment variable expansion (in-place helper) */
    expand_env_vars_inplace(dup_argv);

    /* Builtins */
    if (strcmp(dup_argv[0], "exit") == 0) {
        char *cmdline = join_argv(dup_argv);
        if (cmdline) add_to_history(cmdline);
        part_eight_shutdown();
        exit(0);
    } else if (strcmp(dup_argv[0], "cd") == 0) {
        if (builtin_cd(dup_argv)) {
            char *cmdline = join_argv(dup_argv);
            if (cmdline) { add_to_history(cmdline); free(cmdline); }
        }
    } else if (strcmp(dup_argv[0], "jobs") == 0) {
        char *cmdline = join_argv(dup_argv);
        if (cmdline) { add_to_history(cmdline); free(cmdline); }
        part_eight_jobs_builtin();
    } else {
        /* External command: find executable and run using exec_external's API */
        char *fullpath = find_executable(dup_argv[0]);
        pid_t child = execute_command(dup_argv, fullpath, background);
        if (child > 0 && background) {
            /* register background job with job bookkeeping */
            pid_t p = child;
            char *cmdline = join_argv(dup_argv);
            if (cmdline) {
                part_eight_add_job(cmdline, &p, 1, p);
                add_to_history(cmdline);
                free(cmdline);
            } else {
                add_to_history(dup_argv[0]);
            }
        } else if (child > 0 && !background) {
            char *cmdline = join_argv(dup_argv);
            if (cmdline) { add_to_history(cmdline); free(cmdline); }
        } else {
            /* execution error */
            char *cmdline = join_argv(dup_argv);
            if (cmdline) { add_to_history(cmdline); free(cmdline); }
        }
        if (fullpath) free(fullpath);
    }

    /* cleanup */
    for (int i = 0; dup_argv[i]; ++i) free(dup_argv[i]);
    free(dup_argv);
    free(argv);
}

int main(void) {
    part_eight_init();

    while (1) {
        print_prompt();

        char *input = get_input();
        if (!input) break;

        /* trim leading/trailing whitespace/newline */
        size_t len = strlen(input);
        while (len > 0 && isspace((unsigned char)input[len-1])) input[--len] = '\0';
        char *start = input;
        while (*start && isspace((unsigned char)*start)) start++;

        if (*start == '\0') { free(input); part_eight_check_jobs(); continue; }

        tokenlist *tokens = get_tokens(start);
        if (!tokens || tokens->size == 0) {
            free(input);
            if (tokens) free_tokens(tokens);
            part_eight_check_jobs();
            continue;
        }

        process_command(tokens);

        free_tokens(tokens);
        free(input);

        /* periodically reap background jobs */
        part_eight_check_jobs();
    }

    part_eight_shutdown();
    return 0;
}

