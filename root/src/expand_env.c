//******************************************************************************************************
//* Name:        expand_env.c                                                                          *
//* Description: Environment-variable expansion utilities for COP4610 Project 1 shell.                 *
//*              Provides two APIs:                                                                    *
//*                - expand_env_vars_dup(char **argv): non-destructive, returns newly                  *
//*                  allocated argv with expansions applied (caller must free).                        *
//*                - expand_env_vars_inplace(char **argv): destructive, replaces                       *
//*                  heap-allocated tokens in-place (caller must ensure ownership).                    *
//*              Behavior:                                                                             *
//*                - Expands tokens that are exactly "$NAME" where NAME follows                        *
//*                  POSIX-like rules (first char alpha or '_', subsequent are                         *
//*                  alnum or '_').                                                                    *
//*                - Unset variables are replaced with the empty string ("").                          *
//* Author:      Katelyna Pastrana                                                                     *
//* Date:        2026-01-24                                                                            *
//* References:                                                                                        *
//*    - COP4610 Project 1 specification (tokenization/expansion requirements)                         *
//*    - getenv(3) man page                                                                            *
//*    - Stephen Brennan, "Write a Shell in C" (guidance on where to call expansion)                   *
//*    - POSIX environment-variable naming conventions                                                 *
//*    - CodeVault YouTube videos                                                                      *
//* Compile:     gcc -std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -o expand_env expand_env.c   *
//* Note:        strdup is POSIX; if compiling on a non-POSIX system, provide an                       *
//*              implementation or replace with malloc+strcpy.                                         *
//******************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <shell.h>

/* Validate environment variable NAME according to POSIX-like rules:
 *  - Non-empty
 *  - First char: alphabetic (A-Z,a-z) or underscore '_'
 *  - Subsequent chars: alphanumeric or underscore
 */
static int is_valid_env_name(const char *name) {
    if (name == NULL || name[0] == '\0') return 0;
    if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) return 0;
    for (const char *p = name + 1; *p; ++p) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
    }
    return 1;
}

/* Free a NULL-terminated argv produced by these helpers.
 * Frees each string and the array itself.
 */
void free_argv(char **argv) {
    if (!argv) return;
    for (size_t i = 0; argv[i] != NULL; ++i) {
        free(argv[i]);
    }
    free(argv);
}

/* Non-destructive expansion:
 * - argv_in: NULL-terminated array of strings (may be statics/literals)
 * - Returns a newly allocated NULL-terminated array of strings with expansions applied.
 *   Caller must free returned array with free_argv().
 *
 * Rules:
 * - If token is exactly "$NAME" and NAME is valid, replace with getenv(NAME) or ""
 *   if unset.
 * - Otherwise copy token unchanged.
 */
char **expand_env_vars_dup(char **argv_in) {
    if (!argv_in) return NULL;

    // Count args
    size_t n = 0;
    while (argv_in[n] != NULL) ++n;

    // Allocate new argv (n + 1 for NULL terminator)
    char **out = calloc(n + 1, sizeof(char *));
    if (!out) return NULL;

    for (size_t i = 0; i < n; ++i) {
        const char *tok = argv_in[i];
        if (tok != NULL && tok[0] == '$' && tok[1] != '\0') {
            const char *name = tok + 1;
            if (is_valid_env_name(name)) {
                const char *val = getenv(name);
                out[i] = strdup(val ? val : "");
                if (!out[i]) {
                    free_argv(out);
                    return NULL;
                }
                continue;
            }
        }
        // default: copy token as-is
        out[i] = strdup(tok ? tok : "");
        if (!out[i]) {
            free_argv(out);
            return NULL;
        }
    }
    out[n] = NULL;
    return out;
}

/* In-place destructive expansion:
 * - Assumes caller "owns" the strings in argv (they are heap-allocated).
 * - Replaces argv[i] with newly allocated strings (from strdup) and frees old string.
 * - Returns 0 on success, -1 on allocation error.
 */
int expand_env_vars_inplace(char **argv) {
    if (!argv) return 0;
    for (size_t i = 0; argv[i] != NULL; ++i) {
        char *tok = argv[i];
        if (tok != NULL && tok[0] == '$' && tok[1] != '\0') {
            const char *name = tok + 1;
            if (is_valid_env_name(name)) {
                const char *val = getenv(name);
                char *replacement = strdup(val ? val : "");
                if (!replacement) return -1; /* allocation error; caller left in inconsistent state */
                free(argv[i]);                /* free old token (must be heap-allocated) */
                argv[i] = replacement;
            }
        }
    }
    return 0;
}

#ifdef DEMO_MAIN
/* Demo main for local testing.
 * To build demo: gcc -std=c11 -Wall -Wextra -O2 -DDEMO_MAIN -o expand_env_demo expand_env.c
 */
int main(void) {
    // Example input (simulate tokenizer output)
    char *tokens[] = {
        strdup("echo"),
        strdup("$USER"),
        strdup("literal"),
        strdup("$UNSET_VAR"), /* assume it's not set */
        strdup("$1BAD"),      /* invalid name (starts with digit) */
        NULL
    };

    printf("Original tokens:\n");
    for (size_t i = 0; tokens[i] != NULL; ++i) {
        printf("  [%zu] \"%s\"\n", i, tokens[i]);
    }

    // Non-destructive expansion
    char **expanded = expand_env_vars_dup(tokens);
    if (!expanded) {
        fprintf(stderr, "expand_env_vars_dup failed\n");
        free_argv(tokens);
        return 1;
    }
    printf("\nExpanded (dup) tokens:\n");
    for (size_t i = 0; expanded[i] != NULL; ++i) {
        printf("  [%zu] \"%s\"\n", i, expanded[i]);
    }

    // Destructive expansion (in-place) - works because we used strdup for tokens
    if (expand_env_vars_inplace(tokens) != 0) {
        fprintf(stderr, "expand_env_vars_inplace failed\n");
    } else {
        printf("\nExpanded (inplace) tokens:\n");
        for (size_t i = 0; tokens[i] != NULL; ++i) {
            printf("  [%zu] \"%s\"\n", i, tokens[i]);
        }
    }

    // Cleanup
    free_argv(expanded);
    free_argv(tokens);

    return 0;
}
#endif
