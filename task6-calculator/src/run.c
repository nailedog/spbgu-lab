#include "rpn.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

typedef struct {
    BigNum** items;
    size_t size;
    size_t capacity;
} BigNumStack;

static BigNumStack* stack_create(void) {
    BigNumStack* stack = (BigNumStack*)malloc(sizeof(BigNumStack));
    if (!stack) return NULL;

    stack->capacity = 16;
    stack->size = 0;
    stack->items = (BigNum**)malloc(stack->capacity * sizeof(BigNum*));
    if (!stack->items) {
        free(stack);
        return NULL;
    }

    return stack;
}

static void stack_free(BigNumStack* stack) {
    if (stack) {
        for (size_t i = 0; i < stack->size; i++) {
            bignum_free(stack->items[i]);
        }
        free(stack->items);
        free(stack);
    }
}

static void stack_push(BigNumStack* stack, BigNum* num) {
    if (stack->size >= stack->capacity) {
        stack->capacity *= 2;
        BigNum** new_items = (BigNum**)realloc(stack->items, stack->capacity * sizeof(BigNum*));
        if (!new_items) return;
        stack->items = new_items;
    }
    stack->items[stack->size++] = num;
}

static BigNum* stack_pop(BigNumStack* stack) {
    if (stack->size == 0) return NULL;
    return stack->items[--stack->size];
}

static size_t stack_size(BigNumStack* stack) {
    return stack->size;
}

static RPNResult create_error(RPNError code, const char* message, int position) {
    RPNResult result;
    result.result = NULL;
    result.error = code;
    result.error_message = strdup(message);
    result.error_position = position;
    return result;
}

static bool is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

RPNResult rpn_evaluate(const char* expression) {
    if (!expression) {
        return create_error(RPN_ERROR_MEMORY, "NULL expression", 0);
    }

    BigNumStack* stack = stack_create();
    if (!stack) {
        return create_error(RPN_ERROR_MEMORY, "Memory allocation failed", 0);
    }

    const char* p = expression;
    int position = 0;

    while (*p) {
        // Пропуск пробелов
        while (*p && isspace((unsigned char)*p)) {
            p++;
            position++;
        }

        if (!*p) break;

        // Проверка на число (может начинаться с '-')
        if (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)*(p + 1)))) {
            const char* start = p;
            int num_start = position;

            // Знак
            if (*p == '-') {
                p++;
                position++;
            }

            while (*p && isdigit((unsigned char)*p)) {
                p++;
                position++;
            }
          
            size_t len = p - start;
            char* num_str = (char*)malloc(len + 1);
            if (!num_str) {
                stack_free(stack);
                return create_error(RPN_ERROR_MEMORY, "Memory allocation failed", position);
            }

            strncpy(num_str, start, len);
            num_str[len] = '\0';

            BigNum* num = bignum_from_string(num_str);
            free(num_str);

            if (!num) {
                stack_free(stack);
                char error_msg[100];
                snprintf(error_msg, sizeof(error_msg), "Invalid number at position %d", num_start);
                return create_error(RPN_ERROR_INVALID_CHAR, error_msg, num_start);
            }

            stack_push(stack, num);
        } else if (is_operator(*p)) {
            char op = *p;
            int op_position = position;
          
            if (op == '/') {
                stack_free(stack);
                return create_error(RPN_ERROR_UNSUPPORTED_OP, "Unsupported operation", op_position);
            }

            if (stack_size(stack) < 2) {
                stack_free(stack);
                return create_error(RPN_ERROR_INSUFFICIENT_OPERANDS, "Insufficient operands for operation", op_position);
            }

            BigNum* b = stack_pop(stack);
            BigNum* a = stack_pop(stack);
            BigNum* result = NULL;

            switch (op) {
                case '+':
                    result = bignum_add(a, b);
                    break;
                case '-':
                    result = bignum_subtract(a, b);
                    break;
                case '*':
                    result = bignum_multiply(a, b);
                    break;
            }

            bignum_free(a);
            bignum_free(b);

            if (!result) {
                stack_free(stack);
                return create_error(RPN_ERROR_MEMORY, "Memory allocation failed during operation", op_position);
            }

            stack_push(stack, result);
            p++;
            position++;
        } else {
            stack_free(stack);
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "Invalid character at position %d", position);
            return create_error(RPN_ERROR_INVALID_CHAR, error_msg, position);
        }
    }

    if (stack_size(stack) == 0) {
        stack_free(stack);
        return create_error(RPN_ERROR_INSUFFICIENT_OPERANDS, "No result", 0);
    }

    if (stack_size(stack) > 1) {
        stack_free(stack);
        return create_error(RPN_ERROR_MISSING_OP, "Operation symbol is missed", 0);
    }

    RPNResult result;
    result.result = stack_pop(stack);
    result.error = RPN_OK;
    result.error_message = NULL;
    result.error_position = -1;

    stack_free(stack);
    return result;
}

void rpn_result_free(RPNResult* result) {
    if (result) {
        if (result->result) {
            bignum_free(result->result);
            result->result = NULL;
        }
        if (result->error_message) {
            free(result->error_message);
            result->error_message = NULL;
        }
    }
}
