#pragma once

#include "arraylist.h"
#include <stdbool.h>

// Основание системы счисления 10^9
// Каждый разряд от 0 до 999,999,999
#define BASE 1000000000

typedef struct {
    ArrayList* digits;  // Разряды числа (младшие разряды в начале)
    bool is_negative;   // Знак числа
} BigNum;

BigNum* bignum_create(void);
BigNum* bignum_from_string(const char* str);
BigNum* bignum_from_int(int64_t value);
void bignum_free(BigNum* num);

char* bignum_to_string(const BigNum* num);

int bignum_compare(const BigNum* a, const BigNum* b);
int bignum_compare_abs(const BigNum* a, const BigNum* b);

BigNum* bignum_add(const BigNum* a, const BigNum* b);
BigNum* bignum_subtract(const BigNum* a, const BigNum* b);
BigNum* bignum_multiply(const BigNum* a, const BigNum* b);

BigNum* bignum_clone(const BigNum* num);
void bignum_normalize(BigNum* num);
bool bignum_is_zero(const BigNum* num);
