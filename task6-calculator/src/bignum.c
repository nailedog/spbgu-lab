#include "bignum.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

BigNum* bignum_create(void) {
    BigNum* num = (BigNum*)malloc(sizeof(BigNum));
    if (!num) return NULL;

    num->digits = arraylist_create();
    if (!num->digits) {
        free(num);
        return NULL;
    }

    num->is_negative = false;
    arraylist_push(num->digits, 0);
    return num;
}

void bignum_free(BigNum* num) {
    if (num) {
        if (num->digits) {
            arraylist_free(num->digits);
        }
        free(num);
    }
}

BigNum* bignum_from_string(const char* str) {
    if (!str || !*str) return NULL;

    BigNum* num = bignum_create();
    if (!num) return NULL;

    arraylist_clear(num->digits);

    // Проверка знака
    size_t start = 0;
    if (str[0] == '-') {
        num->is_negative = true;
        start = 1;
    }

    // Пропуск ведущих нулей
    while (str[start] == '0' && str[start + 1] != '\0') {
        start++;
    }

    size_t len = strlen(str + start);
    if (len == 0 || !isdigit((unsigned char)str[start])) {
        arraylist_push(num->digits, 0);
        num->is_negative = false;
        return num;
    }

    // Парсим число по 9 цифр (BASE = 10^9)
    const char* p = str + start + len;
    while (p > str + start) {
        uint32_t digit = 0;
        uint32_t multiplier = 1;
        int count = 0;

        while (count < 9 && p > str + start) {
            p--;
            if (!isdigit((unsigned char)*p)) {
                bignum_free(num);
                return NULL;
            }
            digit += (*p - '0') * multiplier;
            multiplier *= 10;
            count++;
        }

        arraylist_push(num->digits, digit);
    }

    bignum_normalize(num);
    return num;
}

BigNum* bignum_from_int(int64_t value) {
    BigNum* num = bignum_create();
    if (!num) return NULL;

    arraylist_clear(num->digits);

    if (value < 0) {
        num->is_negative = true;
        value = -value;
    }

    if (value == 0) {
        arraylist_push(num->digits, 0);
        return num;
    }

    while (value > 0) {
        arraylist_push(num->digits, value % BASE);
        value /= BASE;
    }

    return num;
}

char* bignum_to_string(const BigNum* num) {
    if (!num || arraylist_size(num->digits) == 0) {
        char* result = (char*)malloc(2);
        strcpy(result, "0");
        return result;
    }

    // Оценка размера: каждый разряд = до 9 цифр + знак + \0
    size_t max_size = arraylist_size(num->digits) * 9 + 2;
    char* result = (char*)malloc(max_size);
    if (!result) return NULL;

    char* p = result;

    if (num->is_negative && !bignum_is_zero(num)) {
        *p++ = '-';
    }

    // Старший разряд без ведущих нулей
    size_t size = arraylist_size(num->digits);
    int written = sprintf(p, "%u", arraylist_get(num->digits, size - 1));
    p += written;

    // Остальные разряды с ведущими нулями (ровно 9 цифр)
    for (size_t i = size - 1; i > 0; i--) {
        written = sprintf(p, "%09u", arraylist_get(num->digits, i - 1));
        p += written;
    }

    return result;
}

void bignum_normalize(BigNum* num) {
    // Удаляем ведущие нули
    while (arraylist_size(num->digits) > 1 &&
           arraylist_get(num->digits, arraylist_size(num->digits) - 1) == 0) {
        arraylist_pop(num->digits);
    }

    // Если число = 0, знак всегда положительный
    if (arraylist_size(num->digits) == 1 && arraylist_get(num->digits, 0) == 0) {
        num->is_negative = false;
    }
}

bool bignum_is_zero(const BigNum* num) {
    return arraylist_size(num->digits) == 1 && arraylist_get(num->digits, 0) == 0;
}

int bignum_compare_abs(const BigNum* a, const BigNum* b) {
    size_t size_a = arraylist_size(a->digits);
    size_t size_b = arraylist_size(b->digits);

    if (size_a != size_b) {
        return size_a > size_b ? 1 : -1;
    }

    for (size_t i = size_a; i > 0; i--) {
        uint32_t digit_a = arraylist_get(a->digits, i - 1);
        uint32_t digit_b = arraylist_get(b->digits, i - 1);
        if (digit_a != digit_b) {
            return digit_a > digit_b ? 1 : -1;
        }
    }

    return 0;
}

int bignum_compare(const BigNum* a, const BigNum* b) {
    if (a->is_negative != b->is_negative) {
        return a->is_negative ? -1 : 1;
    }

    int cmp = bignum_compare_abs(a, b);
    return a->is_negative ? -cmp : cmp;
}

BigNum* bignum_clone(const BigNum* num) {
    BigNum* clone = bignum_create();
    if (!clone) return NULL;

    arraylist_free(clone->digits);
    clone->digits = arraylist_clone(num->digits);
    clone->is_negative = num->is_negative;

    return clone;
}

static BigNum* bignum_add_abs(const BigNum* a, const BigNum* b) {
    BigNum* result = bignum_create();
    arraylist_clear(result->digits);

    size_t max_size = arraylist_size(a->digits) > arraylist_size(b->digits) ?
                      arraylist_size(a->digits) : arraylist_size(b->digits);

    uint64_t carry = 0;
    for (size_t i = 0; i < max_size || carry; i++) {
        uint64_t sum = carry;
        if (i < arraylist_size(a->digits)) {
            sum += arraylist_get(a->digits, i);
        }
        if (i < arraylist_size(b->digits)) {
            sum += arraylist_get(b->digits, i);
        }

        arraylist_push(result->digits, sum % BASE);
        carry = sum / BASE;
    }

    bignum_normalize(result);
    return result;
}

static BigNum* bignum_subtract_abs(const BigNum* a, const BigNum* b) {
    // Предполагаем, что |a| >= |b|
    BigNum* result = bignum_create();
    arraylist_clear(result->digits);

    int64_t borrow = 0;
    for (size_t i = 0; i < arraylist_size(a->digits); i++) {
        int64_t diff = (int64_t)arraylist_get(a->digits, i) - borrow;
        if (i < arraylist_size(b->digits)) {
            diff -= arraylist_get(b->digits, i);
        }

        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }

        arraylist_push(result->digits, (uint32_t)diff);
    }

    bignum_normalize(result);
    return result;
}

BigNum* bignum_add(const BigNum* a, const BigNum* b) {
    if (a->is_negative == b->is_negative) {
        BigNum* result = bignum_add_abs(a, b);
        result->is_negative = a->is_negative;
        return result;
    }

    // Разные знаки: вычитание
    int cmp = bignum_compare_abs(a, b);
    if (cmp == 0) {
        return bignum_from_int(0);
    }

    BigNum* result;
    if (cmp > 0) {
        result = bignum_subtract_abs(a, b);
        result->is_negative = a->is_negative;
    } else {
        result = bignum_subtract_abs(b, a);
        result->is_negative = b->is_negative;
    }

    return result;
}

BigNum* bignum_subtract(const BigNum* a, const BigNum* b) {
    BigNum* neg_b = bignum_clone(b);
    neg_b->is_negative = !neg_b->is_negative;

    BigNum* result = bignum_add(a, neg_b);

    bignum_free(neg_b);
    return result;
}

BigNum* bignum_multiply(const BigNum* a, const BigNum* b) {
    BigNum* result = bignum_create();
    arraylist_clear(result->digits);

    size_t size_a = arraylist_size(a->digits);
    size_t size_b = arraylist_size(b->digits);
    arraylist_resize(result->digits, size_a + size_b);

    for (size_t i = 0; i < size_a; i++) {
        uint64_t carry = 0;
        uint64_t digit_a = arraylist_get(a->digits, i);

        for (size_t j = 0; j < size_b || carry; j++) {
            uint64_t digit_b = (j < size_b) ? arraylist_get(b->digits, j) : 0;
            uint64_t current = arraylist_get(result->digits, i + j);

            uint64_t prod = current + digit_a * digit_b + carry;
            arraylist_set(result->digits, i + j, prod % BASE);
            carry = prod / BASE;
        }
    }

    result->is_negative = (a->is_negative != b->is_negative);
    bignum_normalize(result);

    return result;
}
