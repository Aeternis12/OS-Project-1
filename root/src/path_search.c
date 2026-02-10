#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LEN 1024

void execute_command(char *command, char **argv)
{
    char *path_env = getenv("PATH");
    char *path_copy;
    char *dir;
    char full_path[MAX_PATH_LEN];

    if (path_env == NULL) 
    {
        fprintf(stderr, "PATH not set\n");
        return;
    }

    path_copy = strdup(path_env);
    if (path_copy == NULL) 
    {
        perror("strdup");
        return;
    }

    dir = strtok(path_copy, ":");
    while (dir != NULL) 
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);

        if (access(full_path, X_OK) == 0) 
        {
            execv(full_path, argv);
            perror("execv");
            free(path_copy);
            return;
        }

        dir = strtok(NULL, ":");
    }

    fprintf(stderr, "%s: command not found\n", command);
    free(path_copy);
}
