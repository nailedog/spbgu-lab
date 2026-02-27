#include "rpn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_SIZE 100000

int main(void) {
    char* input = (char*)malloc(MAX_INPUT_SIZE);
    if (!input) {
        fprintf(stderr, "Memory allocation failed\n");
        return 2;
    }

    if (!fgets(input, MAX_INPUT_SIZE, stdin)) {
        free(input);
        fprintf(stderr, "Failed to read input\n");
        return 2;
    }
  
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }
  
    RPNResult result = rpn_evaluate(input);
    free(input);

    if (result.error != RPN_OK) {
        fprintf(stderr, "%s\n", result.error_message);
        rpn_result_free(&result);
        return 1;
    }

    char* result_str = bignum_to_string(result.result);
    if (!result_str) {
        rpn_result_free(&result);
        fprintf(stderr, "Memory allocation failed\n");
        return 2;
    }

    printf("%s\n", result_str);
    free(result_str);
    rpn_result_free(&result);

    return 0;
}
