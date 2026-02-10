#include "shell.h"
#include "lexer.h"

int main(void) {
    print_prompt();

    char *input = get_input();
    if (!input)
        break;

    tokenlist *tokens = get_tokens(input);
    if (!tokens || tokens->size == 0) {
        free(input);
        free_tokens(tokens);
        continue;
    }
}
