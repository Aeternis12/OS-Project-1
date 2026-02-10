#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shell.h"

char* expand_tilde(char* token){
	if (!token) return NULL;
	if(token[0] != '~'){
		return token;
	}

	char* home = getenv("HOME");
	if(home == NULL) {
		return token;
	}

	if(token[1] == '\0'){
		return strdup(home);
	}

	if(token[1] == '/'){
		char* expanded = malloc(strlen(home) + strlen(token));
		if (!expanded) return NULL;
		strcpy(expanded, home);
		strcat(expanded, token + 1);
		return expanded;
	}

	return token;
}

//int main() {
//    // List of test cases
//    char *test_cases[] = {"~", "~/", "~/Documents", "file~name", "not_a_tilde", NULL};
//    
//    printf("Testing Tilde Expansion:\n");
//    for (int i = 0; test_cases[i] != NULL; i++) {
//        char *input = test_cases[i];
//        char *output = expand_tilde(input);
//        
//        printf("Input:  %s\n", input);
//        printf("Output: %s\n\n", output);
//        
//        // Clean up memory if expansion happened
//        if (output != input) free(output);
//    }
//    return 0;
//}
