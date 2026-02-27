#include "ieee754.h"
#include <stdint.h>

typedef union {
    float f;
    uint32_t bits;
    struct {
        uint32_t mantissa : 23;
        uint32_t exponent : 8;
        uint32_t sign : 1;
    } parts;
} FloatBits;

#define EXPONENT_BIAS 127
#define EXPONENT_MAX 255
#define MANTISSA_MASK 0x7FFFFF
#define IMPLIED_BIT 0x800000

static int is_nan(FloatBits fb) {
    return fb.parts.exponent == EXPONENT_MAX && fb.parts.mantissa != 0;
}

static int is_inf(FloatBits fb) {
    return fb.parts.exponent == EXPONENT_MAX && fb.parts.mantissa == 0;
}

static int is_zero(FloatBits fb) {
    return fb.parts.exponent == 0 && fb.parts.mantissa == 0;
}

static int is_denormal(FloatBits fb) {
    return fb.parts.exponent == 0 && fb.parts.mantissa != 0;
}

static FloatBits make_nan(void) {
    FloatBits result;
    result.parts.sign = 0;
    result.parts.exponent = EXPONENT_MAX;
    result.parts.mantissa = 1;
    return result;
}

static FloatBits make_inf(uint32_t sign) {
    FloatBits result;
    result.parts.sign = sign;
    result.parts.exponent = EXPONENT_MAX;
    result.parts.mantissa = 0;
    return result;
}

static FloatBits make_zero(uint32_t sign) {
    FloatBits result;
    result.parts.sign = sign;
    result.parts.exponent = 0;
    result.parts.mantissa = 0;
    return result;
}

float float_sum(float a, float b) {
    FloatBits fa, fb, result;
    fa.f = a;
    fb.f = b;

    // Handle NaN
    if (is_nan(fa) || is_nan(fb)) {
        return make_nan().f;
    }

    // Handle infinity
    if (is_inf(fa)) {
        if (is_inf(fb) && fa.parts.sign != fb.parts.sign) {
            return make_nan().f; // inf - inf = NaN
        }
        return fa.f;
    }
    if (is_inf(fb)) {
        return fb.f;
    }

    // Handle zero
    if (is_zero(fa)) {
        if (is_zero(fb)) {
            // -0 + -0 = -0, otherwise +0
            if (fa.parts.sign && fb.parts.sign) {
                return make_zero(1).f;
            }
            return make_zero(0).f;
        }
        return fb.f;
    }
    if (is_zero(fb)) {
        return fa.f;
    }

    // Extract components
    uint32_t sign_a = fa.parts.sign;
    uint32_t sign_b = fb.parts.sign;
    int32_t exp_a = fa.parts.exponent;
    int32_t exp_b = fb.parts.exponent;
    uint64_t mant_a = fa.parts.mantissa;
    uint64_t mant_b = fb.parts.mantissa;

    // Handle denormal numbers
    if (!is_denormal(fa)) {
        mant_a |= IMPLIED_BIT;
    } else {
        exp_a = 1;
    }

    if (!is_denormal(fb)) {
        mant_b |= IMPLIED_BIT;
    } else {
        exp_b = 1;
    }

    // Align exponents
    int32_t exp_diff = exp_a - exp_b;
    int32_t result_exp;

    if (exp_diff > 0) {
        if (exp_diff > 31) {
            return fa.f; // b is too small
        }
        mant_b >>= exp_diff;
        result_exp = exp_a;
    } else if (exp_diff < 0) {
        if (exp_diff < -31) {
            return fb.f; // a is too small
        }
        mant_a >>= -exp_diff;
        result_exp = exp_b;
    } else {
        result_exp = exp_a;
    }

    // Perform addition/subtraction
    uint64_t result_mant;
    uint32_t result_sign;

    if (sign_a == sign_b) {
        result_mant = mant_a + mant_b;
        result_sign = sign_a;

        // Check for overflow
        if (result_mant & 0x1000000) {
            result_mant >>= 1;
            result_exp++;
        }
    } else {
        if (mant_a >= mant_b) {
            result_mant = mant_a - mant_b;
            result_sign = sign_a;
        } else {
            result_mant = mant_b - mant_a;
            result_sign = sign_b;
        }

        // Normalize
        if (result_mant == 0) {
            return make_zero(0).f;
        }

        while (result_mant && !(result_mant & IMPLIED_BIT)) {
            result_mant <<= 1;
            result_exp--;
        }
    }

    // Check for overflow to infinity
    if (result_exp >= EXPONENT_MAX) {
        return make_inf(result_sign).f;
    }

    // Check for underflow to zero or denormal
    if (result_exp <= 0) {
        if (result_exp < -23) {
            return make_zero(result_sign).f;
        }
        // Denormal number
        result_mant >>= (1 - result_exp);
        result.parts.sign = result_sign;
        result.parts.exponent = 0;
        result.parts.mantissa = result_mant & MANTISSA_MASK;
        return result.f;
    }

    // Build result
    result.parts.sign = result_sign;
    result.parts.exponent = result_exp;
    result.parts.mantissa = result_mant & MANTISSA_MASK;

    return result.f;
}

float float_mul(float a, float b) {
    FloatBits fa, fb, result;
    fa.f = a;
    fb.f = b;

    // Handle NaN
    if (is_nan(fa) || is_nan(fb)) {
        return make_nan().f;
    }

    // Handle zero
    if (is_zero(fa) || is_zero(fb)) {
        if (is_inf(fa) || is_inf(fb)) {
            return make_nan().f; // 0 * inf = NaN
        }
        return make_zero(fa.parts.sign ^ fb.parts.sign).f;
    }

    // Handle infinity
    if (is_inf(fa) || is_inf(fb)) {
        return make_inf(fa.parts.sign ^ fb.parts.sign).f;
    }

    // Extract components
    uint32_t result_sign = fa.parts.sign ^ fb.parts.sign;
    int32_t exp_a = fa.parts.exponent;
    int32_t exp_b = fb.parts.exponent;
    uint64_t mant_a = fa.parts.mantissa;
    uint64_t mant_b = fb.parts.mantissa;

    // Handle denormal numbers
    if (!is_denormal(fa)) {
        mant_a |= IMPLIED_BIT;
    } else {
        exp_a = 1;
    }

    if (!is_denormal(fb)) {
        mant_b |= IMPLIED_BIT;
    } else {
        exp_b = 1;
    }

    // Multiply mantissas
    uint64_t result_mant = (mant_a * mant_b) >> 23;
    int32_t result_exp = exp_a + exp_b - EXPONENT_BIAS;

    // Normalize
    if (result_mant & 0x1000000) {
        result_mant >>= 1;
        result_exp++;
    }

    // Check for overflow
    if (result_exp >= EXPONENT_MAX) {
        return make_inf(result_sign).f;
    }

    // Check for underflow
    if (result_exp <= 0) {
        if (result_exp < -23) {
            return make_zero(result_sign).f;
        }
        // Denormal number
        result_mant >>= (1 - result_exp);
        result.parts.sign = result_sign;
        result.parts.exponent = 0;
        result.parts.mantissa = result_mant & MANTISSA_MASK;
        return result.f;
    }

    result.parts.sign = result_sign;
    result.parts.exponent = result_exp;
    result.parts.mantissa = result_mant & MANTISSA_MASK;

    return result.f;
}

float float_div(float a, float b) {
    FloatBits fa, fb, result;
    fa.f = a;
    fb.f = b;

    // Handle NaN
    if (is_nan(fa) || is_nan(fb)) {
        return make_nan().f;
    }

    // Handle division by zero
    if (is_zero(fb)) {
        if (is_zero(fa)) {
            return make_nan().f; // 0 / 0 = NaN
        }
        return make_inf(fa.parts.sign ^ fb.parts.sign).f;
    }

    // Handle zero dividend
    if (is_zero(fa)) {
        return make_zero(fa.parts.sign ^ fb.parts.sign).f;
    }

    // Handle infinity
    if (is_inf(fa)) {
        if (is_inf(fb)) {
            return make_nan().f; // inf / inf = NaN
        }
        return make_inf(fa.parts.sign ^ fb.parts.sign).f;
    }
    if (is_inf(fb)) {
        return make_zero(fa.parts.sign ^ fb.parts.sign).f;
    }

    // Extract components
    uint32_t result_sign = fa.parts.sign ^ fb.parts.sign;
    int32_t exp_a = fa.parts.exponent;
    int32_t exp_b = fb.parts.exponent;
    uint64_t mant_a = fa.parts.mantissa;
    uint64_t mant_b = fb.parts.mantissa;

    // Handle denormal numbers
    if (!is_denormal(fa)) {
        mant_a |= IMPLIED_BIT;
    } else {
        exp_a = 1;
    }

    if (!is_denormal(fb)) {
        mant_b |= IMPLIED_BIT;
    } else {
        exp_b = 1;
    }

    // Divide mantissas
    uint64_t result_mant = (mant_a << 23) / mant_b;
    int32_t result_exp = exp_a - exp_b + EXPONENT_BIAS;

    // Normalize
    if (!(result_mant & IMPLIED_BIT)) {
        result_mant <<= 1;
        result_exp--;
    }

    // Check for overflow
    if (result_exp >= EXPONENT_MAX) {
        return make_inf(result_sign).f;
    }

    // Check for underflow
    if (result_exp <= 0) {
        if (result_exp < -23) {
            return make_zero(result_sign).f;
        }
        // Denormal number
        result_mant >>= (1 - result_exp);
        result.parts.sign = result_sign;
        result.parts.exponent = 0;
        result.parts.mantissa = result_mant & MANTISSA_MASK;
        return result.f;
    }

    result.parts.sign = result_sign;
    result.parts.exponent = result_exp;
    result.parts.mantissa = result_mant & MANTISSA_MASK;

    return result.f;
}
