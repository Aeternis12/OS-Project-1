#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    char hostname[256];
    char *user = getenv("USER");
    char *pwd = getenv("PWD");

    if (gethostname(hostname, sizeof(hostname)) != 0)
        snprintf(hostname, sizeof(hostname), "unknown");

    if (user == NULL) user = "unknown";
    if (pwd == NULL) pwd = "/";

    printf("%s@%s:%s> ", user, hostname, pwd);
    fflush(stdout);

    return 0;
}
