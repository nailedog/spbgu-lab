#pragma once

#include "bignum.h"

// Коды ошибок
typedef enum {
    RPN_OK = 0,
    RPN_ERROR_INVALID_CHAR = 1,
    RPN_ERROR_UNSUPPORTED_OP = 1,
    RPN_ERROR_MISSING_OP = 1,
    RPN_ERROR_INSUFFICIENT_OPERANDS = 1,
    RPN_ERROR_TOO_MANY_OPERANDS = 1,
    RPN_ERROR_MEMORY = 2
} RPNError;

// Результат вычисления
typedef struct {
    BigNum* result;
    RPNError error;
    char* error_message;
    int error_position;
} RPNResult;

// Вычисление RPN выражения
RPNResult rpn_evaluate(const char* expression);

// Освобождение результата
void rpn_result_free(RPNResult* result);
