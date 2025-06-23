/*
** The MIT License (MIT)
** 
** Copyright (c) 2025 luau-project [https://github.com/luau-project/lua-pcg](https://github.com/luau-project/lua-pcg)
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
*/

#include "lua-pcg.h"

#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LUA_PCG_METATABLE "lua_pcg_metatable"
#define LUA_PCG_PCG32_METATABLE "lua_pcg_pcg32_metatable"
#define LUA_PCG_PCG64_METATABLE "lua_pcg_pcg64_metatable"

typedef unsigned char lua_pcg_u8;
#define lua_pcg_u8_cast(x) ((lua_pcg_u8)((x) & 0xFF))

typedef unsigned short int lua_pcg_u16;
#define lua_pcg_u16_cast(x) ((lua_pcg_u16)((x) & 0xFFFF))

#if (((UINT_MAX >> 15) >> 15) >= 3)
/* unsigned int has at least 32-bits */
typedef unsigned int lua_pcg_u32;
#else
typedef unsigned long int lua_pcg_u32;
#endif

/* macro to create an unsigned int bound to 32-bits */
#define lua_pcg_u32_cast(x) ((lua_pcg_u32)((x) & 0xFFFFFFFF))

/* macro to create a lua_pcg_u32 from low and high, bound to 32-bits */
#define lua_pcg_u32_lh(l,h) ((lua_pcg_u32)((l) & 0xFFFF) | ((((lua_pcg_u32)(h)) & 0xFFFF) << 16))

/*
** the macro below (LUA_PCG_FORCE_U64_EMULATED),
** when defined, has the purpose to explicitly test
** the custom uint64_t emulation
*/

#ifndef LUA_PCG_FORCE_U64_EMULATED /* try to find a 64-bit integer type */
#if (((ULONG_MAX >> 31) >> 31) >= 3)
/* unsigned long has at least 64-bits */
typedef unsigned long int lua_pcg_u64;
#define lua_pcg_u64_DEFINED

#elif defined(ULLONG_MAX)
typedef unsigned long long int lua_pcg_u64;
#define lua_pcg_u64_DEFINED

#elif defined(_MSC_VER) && (_MSC_VER < 1800)
typedef unsigned __int64 lua_pcg_u64;
#define lua_pcg_u64_DEFINED
#endif
#endif

#ifdef lua_pcg_u64_DEFINED

/* macro to create a lua_pcg_u64 bound to 64-bits */
#define lua_pcg_u64_cast(x) ((lua_pcg_u64)((x) & 0xFFFFFFFFFFFFFFFF))

/* macro to create a lua_pcg_u64 from low and high, bound to 64-bits */
#define lua_pcg_u64_lh(l,h) ((lua_pcg_u64)((l) & 0xFFFFFFFF) | ((((lua_pcg_u64)(h)) & 0xFFFFFFFF) << 32))

#else /* start of 64-bit integer emulation */
/*
** 
** Let's emulate 64-bit integer support
** through low and high 32-bit integers.
** The basic idea is: low | (high << 32).
** 
** Most of 64-bit integer emulation
** is pretty straightforward. The
** most complicated parts lie
** on two operations: 1) multiplication
** and 2) division.
** 
** 1. Multiplication: we
** follow the same idea
** that we use to multiply
** integers on paper, often called
** "schoolbook multiplication".
** 
** 2. Division: the idea
** to divide a per b (a / b) has
** the goal to find unique q and r
** satisfying a = q * b + r
** such that 0 <= r < b. To do that,
** we operate candidate values (q * b)
** on 128-bit with q varying from 1
** to 0xFFFFFFFFFFFFFFFF (max value from uint64_t). 
** Since the process to iterate
** through all these values would
** take a lot of time, we employ the bisection
** method to find such q. Thus, the division
** operation has O(N) time complexity,
** where N is the number of bits (64) on uint64_t.
** 
*/
typedef struct
{
    lua_pcg_u32 low;
    lua_pcg_u32 high;
} lua_pcg_u64;

static lua_pcg_u64 lua_pcg_u64_lh(lua_pcg_u32 low, lua_pcg_u32 high)
{
    lua_pcg_u64 res;
    res.low = lua_pcg_u32_cast(low);
    res.high = lua_pcg_u32_cast(high);
    return res;
}

/*
** We need lua_pcg_u32 low and high during
** an extended multiplication (a * b)
** when both a and b are 32-bit integers
** and the result must be a lua_pcg_u64.
*/
typedef struct
{
    lua_pcg_u16 low;
    lua_pcg_u16 high;
} lua_pcg_u32_aux;

/*
** In order to perform an extended multiplication (a * b)
** when both a and b are 64-bit integers,
** we store the result in a 128-bit integer.
*/
typedef struct
{
    lua_pcg_u64 low;
    lua_pcg_u64 high;
} lua_pcg_u128;

/* create a lua_pcg_u128 from low and high, bound to 128-bits */
static lua_pcg_u128 lua_pcg_u128_lh(lua_pcg_u64 l, lua_pcg_u64 h)
{
    lua_pcg_u128 result;
    result.low = lua_pcg_u64_lh(l.low, l.high);
    result.high = lua_pcg_u64_lh(h.low, h.high);
    return result;
}

#define LUA_PCG_U64_EMULATED
#define LUA_PCG_U128_EMULATED
#endif

#ifdef LUA_PCG_U64_EMULATED
/* sum (64-bit): a + b */
static lua_pcg_u64 lua_pcg_u64_sum(lua_pcg_u64 a, lua_pcg_u64 b)
{
    lua_pcg_u64 result = lua_pcg_u64_lh(a.low + b.low, a.high + b.high);
    if (result.low < a.low)
    {
        result.high++;
    }
    return result;
}

/* cast to lua_pcg_u8: (lua_pcg_u8)value */
static lua_pcg_u8 lua_pcg_u64_cast_to_u8(lua_pcg_u64 value)
{
    return lua_pcg_u8_cast(value.low);
}

/* cast to lua_pcg_u16: (lua_pcg_u16)value */
static lua_pcg_u16 lua_pcg_u64_cast_to_u16(lua_pcg_u64 value)
{
    return lua_pcg_u16_cast(value.low);
}

/* cast to lua_pcg_u32: (lua_pcg_u32)value */
static lua_pcg_u32 lua_pcg_u64_cast_to_u32(lua_pcg_u64 value)
{
    return lua_pcg_u32_cast(value.low);
}

/* less than (64-bit): a < b */
#define lua_pcg_u64_lt(a,b) (((a.high) < (b.high)) || (((a.high) == (b.high)) && ((a.low) < (b.low))))

/* greater than (64-bit): a > b */
#define lua_pcg_u64_gt(a,b) (((a.high) > (b.high)) || (((a.high) == (b.high)) && ((a.low) > (b.low))))

/* equal (64-bit): a == b */
#define lua_pcg_u64_eq(a,b) (((a.low) == (b.low)) && ((a.high) == (b.high)))

/* not equal (64-bit): a != b */
#define lua_pcg_u64_neq(a,b) (!(lua_pcg_u64_eq((a),(b))))

/* less than or equal (64-bit): a <= b */
#define lua_pcg_u64_le(a,b) (!(lua_pcg_u64_gt((a),(b))))

/* greater than or equal (64-bit): a >= b */
#define lua_pcg_u64_ge(a,b) (!(lua_pcg_u64_lt((a),(b))))

/* bitwise xor: a ^ b */
static lua_pcg_u64 lua_pcg_u64_xor(lua_pcg_u64 a, lua_pcg_u64 b)
{
    return lua_pcg_u64_lh(((a.low) ^ (b.low)), ((a.high) ^ (b.high)));
}

/* bitwise not: ~ value */
static lua_pcg_u64 lua_pcg_u64_bnot(lua_pcg_u64 value)
{
    return lua_pcg_u64_lh(~(value.low), ~(value.high));
}

/* additive inverse: - value */
static lua_pcg_u64 lua_pcg_u64_additive_inverse(lua_pcg_u64 value)
{
    /* - value <-> (~ value) + 1 */
    return lua_pcg_u64_sum(lua_pcg_u64_bnot(value), lua_pcg_u64_lh(1U, 0U));
}

/* subtraction: a - b */
static lua_pcg_u64 lua_pcg_u64_subtraction(lua_pcg_u64 a, lua_pcg_u64 b)
{
    return lua_pcg_u64_sum(a, lua_pcg_u64_additive_inverse(b));
}

/* bitwise and: a & b */
static lua_pcg_u64 lua_pcg_u64_band(lua_pcg_u64 a, lua_pcg_u64 b)
{
    return lua_pcg_u64_lh(((a.low) & (b.low)), ((a.high) & (b.high)));
}

/* bitwise or: a | b */
static lua_pcg_u64 lua_pcg_u64_bor(lua_pcg_u64 a, lua_pcg_u64 b)
{
    return lua_pcg_u64_lh(((a.low) | (b.low)), ((a.high) | (b.high)));
}

/* right shift: value >> n (0 < n < 64) */
static lua_pcg_u64 lua_pcg_u64_rsh(lua_pcg_u64 value, unsigned int n)
{
    lua_pcg_u64 result;
    if (n <= 0)
    {
        result.low = lua_pcg_u32_cast(value.low);
        result.high = lua_pcg_u32_cast(value.high);
    }
    else if (n >= 64)
    {
        result.low = (lua_pcg_u32)0U;
        result.high = (lua_pcg_u32)0U;
    }
    else if (n == 32)
    {
        result.low = lua_pcg_u32_cast(value.high);
        result.high = (lua_pcg_u32)0U;
    }
    else if (n < 32)
    {
        result.low = (lua_pcg_u32_cast(value.low) >> n) | (lua_pcg_u32_cast(value.high << (32U - n)));
        result.high = lua_pcg_u32_cast(value.high) >> n;
    }
    else /* 32 < n < 64 */
    {
        result.low = lua_pcg_u32_cast(value.high) >> (n - 32U);
        result.high = (lua_pcg_u32)0U;
    }
    return result;
}

/* left shift: value << n (0 < n < 64) */
static lua_pcg_u64 lua_pcg_u64_lsh(lua_pcg_u64 value, unsigned int n)
{
    lua_pcg_u64 result;
    if (n <= 0)
    {
        result.low = lua_pcg_u32_cast(value.low);
        result.high = lua_pcg_u32_cast(value.high);
    }
    else if (n >= 64)
    {
        result.low = (lua_pcg_u32)0U;
        result.high = (lua_pcg_u32)0U;
    }
    else if (n == 32)
    {
        result.high = lua_pcg_u32_cast(value.low);
        result.low = (lua_pcg_u32)0U;
    }
    else if (n < 32)
    {
        result.high = (lua_pcg_u32_cast(value.high << n)) | (lua_pcg_u32_cast(value.low) >> (32U - n));
        result.low = lua_pcg_u32_cast(value.low << n);
    }
    else /* 32 < n < 64 */
    {
        result.high = lua_pcg_u32_cast(value.low << (n - 32U));
        result.low = (lua_pcg_u32)0U;
    }
    return result;
}

/* fills res with low and high parts from value */
static void lua_pcg_u32_aux_set(lua_pcg_u32 value, lua_pcg_u32_aux *res)
{
    res->low = lua_pcg_u16_cast(value);
    res->high = lua_pcg_u16_cast(value >> 16U);
}

/* extended multiplication (32-bit) with 64-bit result: a * b */
static lua_pcg_u64 lua_pcg_u32_mul_ex(lua_pcg_u32 a, lua_pcg_u32 b)
{
    lua_pcg_u64 result;
    lua_pcg_u32 carry;
    lua_pcg_u32 aux;
    lua_pcg_u32_aux aux32, a_aux, b_aux;
    lua_pcg_u16 res_words[4];
    lua_pcg_u16 a_mul_b_low_words[4];
    lua_pcg_u16 a_mul_b_high_words[4];

    /*
    ** fills a_aux and b_aux with high and low
    ** from a and b, respectively
    */
    lua_pcg_u32_aux_set(a, &a_aux);
    lua_pcg_u32_aux_set(b, &b_aux);

    /* compute: a * (b low) */
    aux = ((lua_pcg_u32)a_aux.low) * ((lua_pcg_u32)b_aux.low);
    lua_pcg_u32_aux_set(aux, &aux32);
    a_mul_b_low_words[0] = aux32.low;

    aux = ((lua_pcg_u32)a_aux.high) * ((lua_pcg_u32)b_aux.low) + aux32.high;
    lua_pcg_u32_aux_set(aux, &aux32);
    a_mul_b_low_words[1] = aux32.low;

    a_mul_b_low_words[2] = aux32.high;
    a_mul_b_low_words[3] = (lua_pcg_u16)0U;

    /* compute: a * (b high) */
    aux = ((lua_pcg_u32)a_aux.low) * ((lua_pcg_u32)b_aux.high);
    lua_pcg_u32_aux_set(aux, &aux32);
    a_mul_b_high_words[1] = aux32.low;

    aux = ((lua_pcg_u32)a_aux.high) * ((lua_pcg_u32)b_aux.high) + aux32.high;
    lua_pcg_u32_aux_set(aux, &aux32);
    a_mul_b_high_words[2] = aux32.low;

    a_mul_b_high_words[3] = aux32.high;
    a_mul_b_high_words[0] = (lua_pcg_u16)0U;

    /* sum: a * (b low) and a * (b high) */
    aux = ((lua_pcg_u32)a_mul_b_low_words[0]) + ((lua_pcg_u32)a_mul_b_high_words[0]);
    lua_pcg_u32_aux_set(aux, &aux32);
    res_words[0] = aux32.low;
    carry = aux32.high;

    aux = ((lua_pcg_u32)a_mul_b_low_words[1]) + ((lua_pcg_u32)a_mul_b_high_words[1]) + carry;
    lua_pcg_u32_aux_set(aux, &aux32);
    res_words[1] = aux32.low;
    carry = aux32.high;

    aux = ((lua_pcg_u32)a_mul_b_low_words[2]) + ((lua_pcg_u32)a_mul_b_high_words[2]) + carry;
    lua_pcg_u32_aux_set(aux, &aux32);
    res_words[2] = aux32.low;
    carry = aux32.high;

    aux = ((lua_pcg_u32)a_mul_b_low_words[3]) + ((lua_pcg_u32)a_mul_b_high_words[3]) + carry;
    lua_pcg_u32_aux_set(aux, &aux32);
    res_words[3] = aux32.low;

    /* construct the result */
    result.low = lua_pcg_u32_lh(res_words[0], res_words[1]);
    result.high = lua_pcg_u32_lh(res_words[2], res_words[3]);
    return result;
}

/* multiplication (64-bit) with 64-bit result: a * b */
static lua_pcg_u64 lua_pcg_u64_mul(lua_pcg_u64 a, lua_pcg_u64 b)
{
    lua_pcg_u64 carry;
    lua_pcg_u64 aux;
    lua_pcg_u32 res_words[2];
    lua_pcg_u32 a_mul_b_low_words[2];
    lua_pcg_u32 a_mul_b_high_words[2];

    /* compute: a * (b low) */
    aux = lua_pcg_u32_mul_ex(a.low, b.low);
    a_mul_b_low_words[0] = aux.low;

    aux = lua_pcg_u64_sum(lua_pcg_u32_mul_ex(a.high, b.low), lua_pcg_u64_lh(aux.high, 0U));
    a_mul_b_low_words[1] = aux.low;

    /* compute: a * (b high) */
    aux = lua_pcg_u32_mul_ex(a.low, b.high);
    a_mul_b_high_words[1] = aux.low;
    a_mul_b_high_words[0] = (lua_pcg_u32)0U;

    /* sum: a * (b low) and a * (b high) */
    aux = lua_pcg_u64_sum(
        lua_pcg_u64_lh(a_mul_b_low_words[0], 0U),
        lua_pcg_u64_lh(a_mul_b_high_words[0], 0U)
    );
    res_words[0] = aux.low;
    carry = lua_pcg_u64_lh(aux.high, 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[1], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[1], 0U)
        ),
        carry
    );
    res_words[1] = aux.low;

    /* construct the result */
    return lua_pcg_u64_lh(res_words[0], res_words[1]);
}

/* extended multiplication (64-bit) with 128-bit result: a * b */
static lua_pcg_u128 lua_pcg_u64_mul_ex(lua_pcg_u64 a, lua_pcg_u64 b)
{
    lua_pcg_u128 result;
    lua_pcg_u64 carry;
    lua_pcg_u64 aux;
    lua_pcg_u32 res_words[4];
    lua_pcg_u32 a_mul_b_low_words[4];
    lua_pcg_u32 a_mul_b_high_words[4];

    /* compute: a * (b low) */
    aux = lua_pcg_u32_mul_ex(a.low, b.low);
    a_mul_b_low_words[0] = aux.low;

    aux = lua_pcg_u64_sum(lua_pcg_u32_mul_ex(a.high, b.low), lua_pcg_u64_lh(aux.high, 0U));
    a_mul_b_low_words[1] = aux.low;

    a_mul_b_low_words[2] = aux.high;
    a_mul_b_low_words[3] = (lua_pcg_u32)0U;

    /* compute: a * (b high) */
    aux = lua_pcg_u32_mul_ex(a.low, b.high);
    a_mul_b_high_words[1] = aux.low;

    aux = lua_pcg_u64_sum(lua_pcg_u32_mul_ex(a.high, b.high), lua_pcg_u64_lh(aux.high, 0U));
    a_mul_b_high_words[2] = aux.low;

    a_mul_b_high_words[3] = aux.high;
    a_mul_b_high_words[0] = (lua_pcg_u32)0U;

    /* sum: a * (b low) and a * (b high) */
    aux = lua_pcg_u64_sum(
        lua_pcg_u64_lh(a_mul_b_low_words[0], 0U),
        lua_pcg_u64_lh(a_mul_b_high_words[0], 0U)
    );
    res_words[0] = aux.low;
    carry = lua_pcg_u64_lh(aux.high, 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[1], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[1], 0U)
        ),
        carry
    );
    res_words[1] = aux.low;
    carry = lua_pcg_u64_lh(aux.high, 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[2], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[2], 0U)
        ),
        carry
    );
    res_words[2] = aux.low;
    carry = lua_pcg_u64_lh(aux.high, 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[3], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[3], 0U)
        ),
        carry
    );
    res_words[3] = aux.low;

    /* construct the result */
    result.low = lua_pcg_u64_lh(res_words[0], res_words[1]);
    result.high = lua_pcg_u64_lh(res_words[2], res_words[3]);
    return result;
}
#endif /* end of 64-bit emulation */

#ifndef LUA_PCG_U64_EMULATED
#define lua_pcg_u64_cast_to_u8(v) (lua_pcg_u8_cast((v)))
#define lua_pcg_u64_cast_to_u16(v) (lua_pcg_u16_cast((v)))
#define lua_pcg_u64_cast_to_u32(v) (lua_pcg_u32_cast((v)))
#endif

#ifndef LUA_PCG_FORCE_U128_EMULATED
#if ((!defined(LUA_PCG_U64_EMULATED)) && (defined(__SIZEOF_INT128__)))
typedef unsigned __int128 lua_pcg_u128;
#define lua_pcg_u128_DEFINED

/* macro to create a lua_pcg_u128 from low and high, bound to 128-bits */
#define lua_pcg_u128_lh(l,h) ((lua_pcg_u128)((l) & 0xFFFFFFFFFFFFFFFF) | ((((lua_pcg_u128)(h)) & 0xFFFFFFFFFFFFFFFF) << 64))
#endif
#endif

#ifndef lua_pcg_u128_DEFINED

/* a few definitions before 128-bit emulation */
#ifdef LUA_PCG_U64_EMULATED
#define lua_pcg_u64_clone(v) (lua_pcg_u64_lh((v.low),(v.high)))
#else
#define lua_pcg_u64_clone lua_pcg_u64_cast

/* 
** Emulate a 128-bit integer through low and high.
** The basic idea is that:
** value = low | high << 64
*/
typedef struct
{
    lua_pcg_u64 low;
    lua_pcg_u64 high;
} lua_pcg_u128;

/* create a lua_pcg_u128 from low and high, bound to 128-bits */
static lua_pcg_u128 lua_pcg_u128_lh(lua_pcg_u64 l, lua_pcg_u64 h)
{
    lua_pcg_u128 result;
    result.low = lua_pcg_u64_cast(l);
    result.high = lua_pcg_u64_cast(h);
    return result;
}
#endif

#ifndef LUA_PCG_U128_EMULATED
#define LUA_PCG_U128_EMULATED
#endif
#endif

/* start of 128-bit emulation */
#ifdef LUA_PCG_U128_EMULATED

/* when native 64-bit is available,
** forward operations and methods
** for 128-bit emulation, casting when necessary.
*/
#ifndef LUA_PCG_U64_EMULATED
#define lua_pcg_u64_sum(a,b) (lua_pcg_u64_cast((a)+(b)))
#define lua_pcg_u64_lt(a,b) ((a)<(b))
#define lua_pcg_u64_gt(a,b) ((a)>(b))
#define lua_pcg_u64_eq(a,b) ((a)==(b))
#define lua_pcg_u64_neq(a,b) ((a)!=(b))
#define lua_pcg_u64_le(a,b) ((a)<=(b))
#define lua_pcg_u64_ge(a,b) ((a)>=(b))
#define lua_pcg_u64_xor(a,b) (lua_pcg_u64_cast((a)^(b)))
#define lua_pcg_u64_band(a,b) (lua_pcg_u64_cast((a)&(b)))
#define lua_pcg_u64_bor(a,b) (lua_pcg_u64_cast((a)|(b)))
#define lua_pcg_u64_bnot(v) (lua_pcg_u64_cast(~((lua_pcg_u64)v)))
#define lua_pcg_u64_additive_inverse(v) (lua_pcg_u64_cast((-(v))))
#define lua_pcg_u64_lsh(v,n) (lua_pcg_u64_cast((v)<<(n)))
#define lua_pcg_u64_rsh(v,n) (lua_pcg_u64_cast((v)>>(n)))
#define lua_pcg_u64_subtraction(a,b) (lua_pcg_u64_cast((a)-(b)))
#define lua_pcg_u32_mul_ex(a,b) (lua_pcg_u64_cast(((lua_pcg_u64)a)*((lua_pcg_u64)b)))
#define lua_pcg_u64_mul(a,b) (lua_pcg_u64_cast(((lua_pcg_u64)a)*((lua_pcg_u64)b)))
#define lua_pcg_u64_high(v) (lua_pcg_u32_cast(((v)>>(32))))
#define lua_pcg_u64_low(v) (lua_pcg_u32_cast((v)))

/* extended multiplication (64-bit) with 128-bit result: a * b */
static lua_pcg_u128 lua_pcg_u64_mul_ex(lua_pcg_u64 a, lua_pcg_u64 b)
{
    lua_pcg_u128 result;
    lua_pcg_u64 carry;
    lua_pcg_u64 aux;
    lua_pcg_u32 a_low, b_low, a_high, b_high;
    lua_pcg_u32 res_words[4];
    lua_pcg_u32 a_mul_b_low_words[4];
    lua_pcg_u32 a_mul_b_high_words[4];

    /* compute: a low, a high, b low, b high */
    a_low = lua_pcg_u64_low(a);
    a_high = lua_pcg_u64_high(a);
    b_low = lua_pcg_u64_low(b);
    b_high = lua_pcg_u64_high(b);

    /* compute: a * (b low) */
    aux = lua_pcg_u32_mul_ex(a_low, b_low);
    a_mul_b_low_words[0] = lua_pcg_u64_low(aux);

    aux = lua_pcg_u64_sum(lua_pcg_u32_mul_ex(a_high, b_low), lua_pcg_u64_lh(lua_pcg_u64_high(aux), 0U));
    a_mul_b_low_words[1] = lua_pcg_u64_low(aux);

    a_mul_b_low_words[2] = lua_pcg_u64_high(aux);
    a_mul_b_low_words[3] = (lua_pcg_u32)0U;

    /* compute: a * (b high) */
    aux = lua_pcg_u32_mul_ex(a_low, b_high);
    a_mul_b_high_words[1] = lua_pcg_u64_low(aux);

    aux = lua_pcg_u64_sum(lua_pcg_u32_mul_ex(a_high, b_high), lua_pcg_u64_lh(lua_pcg_u64_high(aux), 0U));
    a_mul_b_high_words[2] = lua_pcg_u64_low(aux);

    a_mul_b_high_words[3] = lua_pcg_u64_high(aux);
    a_mul_b_high_words[0] = (lua_pcg_u32)0U;

    /* sum: a * (b low) and a * (b high) */
    aux = lua_pcg_u64_sum(
        lua_pcg_u64_lh(a_mul_b_low_words[0], 0U),
        lua_pcg_u64_lh(a_mul_b_high_words[0], 0U)
    );
    res_words[0] = lua_pcg_u64_low(aux);
    carry = lua_pcg_u64_lh(lua_pcg_u64_high(aux), 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[1], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[1], 0U)
        ),
        carry
    );
    res_words[1] = lua_pcg_u64_low(aux);
    carry = lua_pcg_u64_lh(lua_pcg_u64_high(aux), 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[2], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[2], 0U)
        ),
        carry
    );
    res_words[2] = lua_pcg_u64_low(aux);
    carry = lua_pcg_u64_lh(lua_pcg_u64_high(aux), 0U);

    aux = lua_pcg_u64_sum(
        lua_pcg_u64_sum(
            lua_pcg_u64_lh(a_mul_b_low_words[3], 0U),
            lua_pcg_u64_lh(a_mul_b_high_words[3], 0U)
        ),
        carry
    );
    res_words[3] = lua_pcg_u64_low(aux);

    /* construct the result */
    result.low = lua_pcg_u64_lh(res_words[0], res_words[1]);
    result.high = lua_pcg_u64_lh(res_words[2], res_words[3]);
    return result;
}
#endif

/* sum (128-bit): a + b */
static lua_pcg_u128 lua_pcg_u128_sum(lua_pcg_u128 a, lua_pcg_u128 b)
{
    lua_pcg_u128 result;
    result.low = lua_pcg_u64_sum(a.low, b.low);
    result.high = lua_pcg_u64_sum(a.high, b.high);
    if (lua_pcg_u64_lt(result.low, a.low))
    {
        result.high = lua_pcg_u64_sum(result.high, lua_pcg_u64_lh(1U, 0U));
    }
    return result;
}

#ifdef LUA_PCG_U64_EMULATED
/* cast lua_pcg_u128 to lua_pcg_u64: (lua_pcg_u64) value */
static lua_pcg_u64 lua_pcg_u128_cast_to_u64(lua_pcg_u128 value)
{
    return lua_pcg_u64_lh((value.low.low), (value.low.high));
}

/* cast lua_pcg_u128 to lua_pcg_u32: (lua_pcg_u32) value */
static lua_pcg_u32 lua_pcg_u128_cast_to_u32(lua_pcg_u128 value)
{
    return lua_pcg_u32_cast(value.low.low);
}

/* cast lua_pcg_u128 to lua_pcg_u16: (lua_pcg_u16) value */
static lua_pcg_u16 lua_pcg_u128_cast_to_u16(lua_pcg_u128 value)
{
    return lua_pcg_u16_cast(value.low.low);
}

/* cast lua_pcg_u128 to lua_pcg_u8: (lua_pcg_u8) value */
static lua_pcg_u8 lua_pcg_u128_cast_to_u8(lua_pcg_u128 value)
{
    return lua_pcg_u8_cast(value.low.low);
}
#else
/* cast lua_pcg_u128 to lua_pcg_u64: (lua_pcg_u64) value */
static lua_pcg_u64 lua_pcg_u128_cast_to_u64(lua_pcg_u128 value)
{
    return lua_pcg_u64_cast((value.low));
}

/* cast lua_pcg_u128 to lua_pcg_u32: (lua_pcg_u32) value */
static lua_pcg_u32 lua_pcg_u128_cast_to_u32(lua_pcg_u128 value)
{
    return lua_pcg_u32_cast((value.low));
}

/* cast lua_pcg_u128 to lua_pcg_u16: (lua_pcg_u16) value */
static lua_pcg_u16 lua_pcg_u128_cast_to_u16(lua_pcg_u128 value)
{
    return lua_pcg_u16_cast((value.low));
}

/* cast lua_pcg_u128 to lua_pcg_u8: (lua_pcg_u8) value */
static lua_pcg_u8 lua_pcg_u128_cast_to_u8(lua_pcg_u128 value)
{
    return lua_pcg_u8_cast((value.low));
}
#endif

/* less than (128-bit): a < b */
#define lua_pcg_u128_lt(a,b) ((lua_pcg_u64_lt((a.high), (b.high))) || ((lua_pcg_u64_eq((a.high), (b.high))) && (lua_pcg_u64_lt((a.low), (b.low)))))

/* greater than (128-bit): a > b */
#define lua_pcg_u128_gt(a,b) ((lua_pcg_u64_gt((a.high), (b.high))) || ((lua_pcg_u64_eq((a.high), (b.high))) && (lua_pcg_u64_gt((a.low), (b.low)))))

/* equal (128-bit): a == b */
#define lua_pcg_u128_eq(a,b) ((lua_pcg_u64_eq((a.low), (b.low))) && (lua_pcg_u64_eq((a.high), (b.high))))

/* not equal (128-bit): a != b */
#define lua_pcg_u128_neq(a,b) (!(lua_pcg_u128_eq((a),(b))))

/* less than or equal (128-bit): a <= b */
#define lua_pcg_u128_le(a,b) (!(lua_pcg_u128_gt((a),(b))))

/* greater than or equal (128-bit): a >= b */
#define lua_pcg_u128_ge(a,b) (!(lua_pcg_u128_lt((a),(b))))

/* bitwise xor: a ^ b */
static lua_pcg_u128 lua_pcg_u128_xor(lua_pcg_u128 a, lua_pcg_u128 b)
{
    return lua_pcg_u128_lh((lua_pcg_u64_xor((a.low), (b.low))), (lua_pcg_u64_xor((a.high), (b.high))));
}

/* bitwise not: ~ value */
static lua_pcg_u128 lua_pcg_u128_bnot(lua_pcg_u128 value)
{
    return lua_pcg_u128_lh((lua_pcg_u64_bnot((value.low))), (lua_pcg_u64_bnot((value.high))));
}

/* additive inverse: - value */
static lua_pcg_u128 lua_pcg_u128_additive_inverse(lua_pcg_u128 value)
{
    /* - value <-> (~ value) + 1 */
    return lua_pcg_u128_sum(lua_pcg_u128_bnot(value), lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U)));
}

/* subtraction: a - b */
static lua_pcg_u128 lua_pcg_u128_subtraction(lua_pcg_u128 a, lua_pcg_u128 b)
{
    return lua_pcg_u128_sum(a, lua_pcg_u128_additive_inverse(b));
}

/* bitwise or: a | b */
static lua_pcg_u128 lua_pcg_u128_bor(lua_pcg_u128 a, lua_pcg_u128 b)
{
    return lua_pcg_u128_lh((lua_pcg_u64_bor((a.low), (b.low))), (lua_pcg_u64_bor((a.high), (b.high))));
}

/* bitwise and: a & b */
static lua_pcg_u128 lua_pcg_u128_band(lua_pcg_u128 a, lua_pcg_u128 b)
{
    return lua_pcg_u128_lh((lua_pcg_u64_band((a.low), (b.low))), (lua_pcg_u64_band((a.high), (b.high))));
}

/* right shift: value >> n (0 < n < 128) */
static lua_pcg_u128 lua_pcg_u128_rsh(lua_pcg_u128 value, unsigned int n)
{
    lua_pcg_u128 result;
    if (n <= 0)
    {
        result.low = lua_pcg_u64_clone(value.low);
        result.high = lua_pcg_u64_clone(value.high);
    }
    else if (n >= 128)
    {
        result.low = lua_pcg_u64_lh(0U, 0U);
        result.high = lua_pcg_u64_lh(0U, 0U);
    }
    else if (n == 64)
    {
        result.low = lua_pcg_u64_clone(value.high);
        result.high = lua_pcg_u64_lh(0U, 0U);
    }
    else if (n < 64)
    {
        result.low = lua_pcg_u64_bor((lua_pcg_u64_rsh(value.low, n)), (lua_pcg_u64_lsh(value.high, (64U - n))));
        result.high = lua_pcg_u64_rsh(value.high, n);
    }
    else /* 64 < n < 128 */
    {
        result.low = lua_pcg_u64_rsh(value.high, (n - 64U));
        result.high = lua_pcg_u64_lh(0U, 0U);
    }
    return result;
}

/* left shift: value << n (0 < n < 128) */
static lua_pcg_u128 lua_pcg_u128_lsh(lua_pcg_u128 value, unsigned int n)
{
    lua_pcg_u128 result;
    if (n <= 0)
    {
        result.low = lua_pcg_u64_clone(value.low);
        result.high = lua_pcg_u64_clone(value.high);
    }
    else if (n >= 128)
    {
        result.low = lua_pcg_u64_lh(0U, 0U);
        result.high = lua_pcg_u64_lh(0U, 0U);
    }
    else if (n == 64)
    {
        result.high = lua_pcg_u64_clone(value.low);
        result.low = lua_pcg_u64_lh(0U, 0U);
    }
    else if (n < 64)
    {
        result.high = lua_pcg_u64_bor((lua_pcg_u64_lsh(value.high, n)), (lua_pcg_u64_rsh(value.low, (64U - n))));
        result.low = lua_pcg_u64_lsh(value.low, n);
    }
    else /* 64 < n < 128 */
    {
        result.high = lua_pcg_u64_lsh(value.low, (n - 64U));
        result.low = lua_pcg_u64_lh(0U, 0U);
    }
    return result;
}

/* multiplication (128-bit) with 128-bit result: a * b */
static lua_pcg_u128 lua_pcg_u128_mul(lua_pcg_u128 a, lua_pcg_u128 b)
{
    lua_pcg_u128 carry;
    lua_pcg_u128 aux;
    lua_pcg_u64 res_words[2];
    lua_pcg_u64 a_mul_b_low_words[2];
    lua_pcg_u64 a_mul_b_high_words[2];
    lua_pcg_u64 _0 = lua_pcg_u64_lh(0U, 0U);

    /* compute: a * (b low) */
    aux = lua_pcg_u64_mul_ex(a.low, b.low);
    a_mul_b_low_words[0] = aux.low;

    aux = lua_pcg_u128_sum(lua_pcg_u64_mul_ex(a.high, b.low), lua_pcg_u128_lh(aux.high, _0));
    a_mul_b_low_words[1] = aux.low;

    /* compute: a * (b high) */
    aux = lua_pcg_u64_mul_ex(a.low, b.high);
    a_mul_b_high_words[1] = aux.low;
    a_mul_b_high_words[0] = lua_pcg_u64_lh(0U, 0U);

    /* sum: a * (b low) and a * (b high) */
    aux = lua_pcg_u128_sum(
        lua_pcg_u128_lh(a_mul_b_low_words[0], _0),
        lua_pcg_u128_lh(a_mul_b_high_words[0], _0)
    );
    res_words[0] = aux.low;
    carry = lua_pcg_u128_lh(aux.high, _0);

    aux = lua_pcg_u128_sum(
        lua_pcg_u128_sum(
            lua_pcg_u128_lh(a_mul_b_low_words[1], _0),
            lua_pcg_u128_lh(a_mul_b_high_words[1], _0)
        ),
        carry
    );
    res_words[1] = aux.low;

    /* construct the result */
    return lua_pcg_u128_lh(res_words[0], res_words[1]);
}

#ifdef LUA_PCG_U64_EMULATED

/* division (64-bit) with 64-bit result: a / b */
static int lua_pcg_u64_div(lua_pcg_u64 a, lua_pcg_u64 b, lua_pcg_u64 *q, lua_pcg_u64 *r)
{
    int res = 0;

    /* variables definitions for the interesting case (a > b) */
    int found = 0;
    lua_pcg_u32 q32, a_low, b_low;
    lua_pcg_u128 _a, _b, _q, q_left, q_right, qb;

    /* b == 0 */
    if (lua_pcg_u64_eq(b, lua_pcg_u64_lh(0U, 0U)))
    {
        res = 1;
    }
    /* b == 1 */
    else if (lua_pcg_u64_eq(b, lua_pcg_u64_lh(1U, 0U)))
    {
        *q = lua_pcg_u64_clone(a);
        *r = lua_pcg_u64_lh(0U, 0U);
    }
    /* a == b */
    else if (lua_pcg_u64_eq(a, b))
    {
        *q = lua_pcg_u64_lh(1U, 0U);
        *r = lua_pcg_u64_lh(0U, 0U);
    }
    /* a < b */
    else if (lua_pcg_u64_lt(a, b))
    {
        *q = lua_pcg_u64_lh(0U, 0U);
        *r = lua_pcg_u64_clone(a);
    }
    else if (a.high == ((lua_pcg_u32)0U) && b.high == ((lua_pcg_u32)0U))  /* a > b, both a and b are 32-bit values */
    {
        a_low = lua_pcg_u32_cast(a.low);
        b_low = lua_pcg_u32_cast(b.low);

        q32 = a_low / b_low; /* q32 = a / b on 32-bit */
        *q = lua_pcg_u64_lh(q32, 0U);
        *r = lua_pcg_u64_lh(a_low - q32 * b_low, 0U); /* r = a - q32 * b */
    }
    else /* a > b, the interesting case with a or b truly 64-bit values */
    {
        /* 
        ** In order to find both (q)uotient and (r)emainder,
        ** q must be the maximum integer value
        ** such that q * b <= a.
        ** 
        ** By definition of q being a maximum,
        ** this means that (q + 1) * b > a, which is the same as
        ** q * b + b > a.
        ** 
        ** So, to find such q, we perform the bisection method
        ** on q between [1, MAX_VALUE], where MAX_VALUE is the
        ** maximum integer value for the type.
        ** 
        ** Thus, since all the other methods involved here are O(1),
        ** the division has O(N) time complexity
        ** where N is the number of bits of the integer type.
        */
        found = 0;

        /* _a = (lua_pcg_u128) a */
        _a = lua_pcg_u128_lh(a, lua_pcg_u64_lh(0U, 0U));

        /* _b = (lua_pcg_u128) b */
        _b = lua_pcg_u128_lh(b, lua_pcg_u64_lh(0U, 0U));

        /* q_left = (lua_pcg_u128) 1U */
        q_left = lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U));

        /* q_right = (lua_pcg_u128) UINT64_MAX */
        q_right = lua_pcg_u128_lh(lua_pcg_u64_lh(0xFFFFFFFF, 0xFFFFFFFF), lua_pcg_u64_lh(0U, 0U));

        do
        {
            /* _q = (q_left + q_right) >> 1U */
            _q = lua_pcg_u128_rsh(lua_pcg_u128_sum(q_left, q_right), 1U);

            /* qb = _q * _b */
            qb = lua_pcg_u128_mul(_q, _b);

            /* qb == _a */
            if (lua_pcg_u128_eq(qb, _a)) /* found an exact divison (a = q * b) */
            {
                q_left = _q;

                /* q_right = _q + ((lua_pcg_u128) 1U) */
                q_right = lua_pcg_u128_sum(_q, lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U)));

                /* *q = (lua_pcg_u64) _q */
                *q = lua_pcg_u128_cast_to_u64(_q);

                /* *r = (lua_pcg_u64) 0U */
                *r = lua_pcg_u64_lh(0U, 0U);

                /* set q and r as found */
                found = 1;
            }
            /* qb < _a */
            else if (lua_pcg_u128_lt(qb, _a))
            {
                /* _a < qb + _b */
                if (lua_pcg_u128_lt(_a, lua_pcg_u128_sum(qb, _b))) /* found a maximum q such that q * b < a */
                {
                    q_left = _q;

                    /* q_right = _q + ((lua_pcg_u128) 1U) */
                    q_right = lua_pcg_u128_sum(_q, lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U)));

                    /* *q = (lua_pcg_u64) _q */
                    *q = lua_pcg_u128_cast_to_u64(_q);

                    /* *r = (lua_pcg_u64) (_a - qb) */
                    *r = lua_pcg_u128_cast_to_u64(lua_pcg_u128_subtraction(_a, qb));

                    /* set q and r as found */
                    found = 1;
                }
                else
                {
                    q_left = _q;
                }
            }
            else /* if (qb > a) */
            {
                q_right = _q;
            }
        }
        while (!found);
    }

    return res;
}

#else /* 64-bit is native */

/* division (64-bit) with 64-bit result: a / b */
static int lua_pcg_u64_div(lua_pcg_u64 a, lua_pcg_u64 b, lua_pcg_u64 *q, lua_pcg_u64 *r)
{
    int res = 0;
    lua_pcg_u64 _a, _b, _q;

    if (b == 0U)
    {
        res = 1;
    }
    else
    {
        _a = lua_pcg_u64_clone(a);
        _b = lua_pcg_u64_clone(b);
        _q = _a / _b;
        *q = _q;
        *r = _a - _q * _b;
    }

    return res;
}

#endif

#endif /* end of 128-bit emulation */

/* final macro definitions */

#ifndef LUA_PCG_U64_EMULATED
#define lua_pcg_u64_lsh(v,n) (lua_pcg_u64_cast((v)<<(n)))
#define lua_pcg_u64_rsh(v,n) (lua_pcg_u64_cast((v)>>(n)))
#define lua_pcg_u64_bor(a,b) (lua_pcg_u64_cast((a)|(b)))
#endif

#ifndef LUA_PCG_U128_EMULATED
#define lua_pcg_u128_lsh(v,n) (((lua_pcg_u128)v)<<(n))
#define lua_pcg_u128_rsh(v,n) (((lua_pcg_u128)v)>>(n))
#define lua_pcg_u128_bor(a,b) (((lua_pcg_u128)a)|((lua_pcg_u128)b))
#define lua_pcg_u128_cast_to_u8(v) (lua_pcg_u8_cast((v)))
#define lua_pcg_u128_cast_to_u16(v) (lua_pcg_u16_cast((v)))
#define lua_pcg_u128_cast_to_u32(v) (lua_pcg_u32_cast((v)))
#define lua_pcg_u128_cast_to_u64(v) (lua_pcg_u64_cast((v)))
#endif

/* start of utility methods to bind Lua functions */

#if LUA_VERSION_NUM < 502
#define lua_pcg_table_length lua_objlen
#else
#define lua_pcg_table_length lua_rawlen
#endif

static int lua_pcg_lua_Integer_has_32bit()
{
    #if LUA_VERSION_NUM < 503
    /* use size_t as a kind of "unsigned ptrdiff_t" */
    return (((((size_t)(~(size_t)0)) >> 15) >> 15) >= 3) ? 1 : 0;
    #else
    return (((((lua_Unsigned)(~(lua_Unsigned)0)) >> 15) >> 15) >= 3) ? 1 : 0;
    #endif
}

static int lua_pcg_lua_Integer_has_64bit()
{
    #if LUA_VERSION_NUM < 503
    /* use size_t as a kind of "unsigned ptrdiff_t" */
    return (((((((size_t)(~(size_t)0)) >> 15) >> 15) >> 15) >> 15) >= 0xF) ? 1 : 0;
    #else
    return (((((((lua_Unsigned)(~(lua_Unsigned)0)) >> 15) >> 15) >> 15) >> 15) >= 0xF) ? 1 : 0;
    #endif
}

#if LUA_VERSION_NUM < 503
static int lua_pcg_aux_isinteger(lua_State *L, int idx)
{
    /*
    ** the body of this function
    ** came from `luaL_checkinteger'
    ** at https://www.lua.org/source/5.1/lauxlib.c.html
    */
    lua_Integer d = lua_tointeger(L, idx);
    return !(d == 0 && !lua_isnumber(L, idx));
}
#else
#define lua_pcg_aux_isinteger lua_isinteger
#endif

/* end of utility methods to bind Lua functions */

/*
** *****************************************************
** *****************************************************
** 
** Now that 64-bit integer support has been emulated,
** and all the operations needed by pcg32 are available,
** we are going to implement pcg32.
** 
** *****************************************************
** *****************************************************
*/

struct lua_pcg_state_setseq_64
{
    lua_pcg_u64 state;
    lua_pcg_u64 inc;
};

/*
** LUA_PCG_DEFAULT_MULTIPLIER_64 = 6364136223846793005
**                               = 0x5851F42D4C957F2D
**                               = 0x4C957F2D | (0x5851F42D << 0x20)
*/
#define LUA_PCG_DEFAULT_MULTIPLIER_64 (lua_pcg_u64_lh(0x4C957F2D,0x5851F42D))

static lua_pcg_u32 lua_pcg_rotr_32(lua_pcg_u32 value, unsigned int rot)
{
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(push)
#pragma warning(disable:4146)
#endif
    return ((lua_pcg_u32_cast(value) >> rot) | (lua_pcg_u32_cast(value << ((-rot) & 31))));
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(pop)
#endif
}

#ifdef LUA_PCG_U64_EMULATED

static lua_pcg_u32 lua_pcg_output_xsh_rr_64_32(lua_pcg_u64 state)
{
    /* lua_pcg_rotr_32(
    **     (lua_pcg_u32)( ((state >> 18U) ^ state) >> 27U ),
    **     (lua_pcg_u32)( state >> 59U )
    ** )
    */
    return lua_pcg_rotr_32(
        lua_pcg_u64_cast_to_u32(lua_pcg_u64_rsh(lua_pcg_u64_xor(lua_pcg_u64_rsh(state, 18U), state), 27U)),
        (unsigned int)lua_pcg_u64_cast_to_u32(lua_pcg_u64_rsh(state, 59U))
    );
}

static void lua_pcg_setseq_64_step_r(struct lua_pcg_state_setseq_64 *rng)
{
    /* rng->state = rng->state * LUA_PCG_DEFAULT_MULTIPLIER_64 + rng->inc */
    rng->state = lua_pcg_u64_sum(lua_pcg_u64_mul(rng->state, LUA_PCG_DEFAULT_MULTIPLIER_64), rng->inc);
}

static lua_pcg_u32 lua_pcg_setseq_64_xsh_rr_32_random_r(struct lua_pcg_state_setseq_64 *rng)
{
    lua_pcg_u64 oldstate = rng->state;
    lua_pcg_setseq_64_step_r(rng);
    return lua_pcg_output_xsh_rr_64_32(oldstate);
}

static void lua_pcg_setseq_64_srandom_r(struct lua_pcg_state_setseq_64 *rng, lua_pcg_u64 initstate, lua_pcg_u64 initseq)
{
    /* rng->state = 0U */
    rng->state = lua_pcg_u64_lh(0U, 0U);

    /* rng->inc = (initseq << 1U) | 1U */
    rng->inc = lua_pcg_u64_bor(lua_pcg_u64_lsh(initseq, 1U), lua_pcg_u64_lh(1U, 0U));

    lua_pcg_setseq_64_step_r(rng);

    /* rng->state = rng->state + initstate */
    rng->state = lua_pcg_u64_sum(rng->state, initstate);

    lua_pcg_setseq_64_step_r(rng);
}

static lua_pcg_u64 lua_pcg_advance_lcg_64(lua_pcg_u64 state, lua_pcg_u64 delta, lua_pcg_u64 cur_mult, lua_pcg_u64 cur_plus)
{
    /* 0U */
    lua_pcg_u64 zero = lua_pcg_u64_lh(0U, 0U);
    /* 1U */
    lua_pcg_u64 one = lua_pcg_u64_lh(1U, 0U);

    /* acc_mult = 1U */
    lua_pcg_u64 acc_mult = lua_pcg_u64_lh(1U, 0U);

    /* acc_plus = 0U */
    lua_pcg_u64 acc_plus = lua_pcg_u64_lh(0U, 0U);

    lua_pcg_u64 _delta = delta;
    lua_pcg_u64 _cur_mult = cur_mult;
    lua_pcg_u64 _cur_plus = cur_plus;

    /* while (delta > 0) */
    while (lua_pcg_u64_gt(_delta, zero))
    {
        /* if (delta & 1) */
        if (lua_pcg_u64_neq(lua_pcg_u64_band(_delta, one), zero))
        {
            /* acc_mult *= cur_mult */
            acc_mult = lua_pcg_u64_mul(acc_mult, _cur_mult);

            /* acc_plus = acc_plus * cur_mult + cur_plus */
            acc_plus = lua_pcg_u64_sum(lua_pcg_u64_mul(acc_plus, _cur_mult), _cur_plus);
        }

        /* cur_plus = (cur_mult + 1) * cur_plus */
        _cur_plus = lua_pcg_u64_mul(lua_pcg_u64_sum(_cur_mult, one), _cur_plus);

        /* cur_mult *= cur_mult */
        _cur_mult = lua_pcg_u64_mul(_cur_mult, _cur_mult);

        /* delta /= 2 which is equal to delta >>= 1 */
        _delta = lua_pcg_u64_rsh(_delta, 1);
    }

    /* return acc_mult * state + acc_plus */
    return lua_pcg_u64_sum(lua_pcg_u64_mul(acc_mult, state), acc_plus);
}

#else

static lua_pcg_u32 lua_pcg_output_xsh_rr_64_32(lua_pcg_u64 state)
{
    return lua_pcg_rotr_32(lua_pcg_u32_cast(((state >> 18U) ^ state) >> 27U), ((unsigned int)(state >> 59U)));
}

static void lua_pcg_setseq_64_step_r(struct lua_pcg_state_setseq_64 *rng)
{
    rng->state = lua_pcg_u64_cast(rng->state * LUA_PCG_DEFAULT_MULTIPLIER_64 + rng->inc);
}

static lua_pcg_u32 lua_pcg_setseq_64_xsh_rr_32_random_r(struct lua_pcg_state_setseq_64 *rng)
{
    lua_pcg_u64 oldstate = rng->state;
    lua_pcg_setseq_64_step_r(rng);
    return lua_pcg_output_xsh_rr_64_32(oldstate);
}

static void lua_pcg_setseq_64_srandom_r(struct lua_pcg_state_setseq_64 *rng, lua_pcg_u64 initstate, lua_pcg_u64 initseq)
{
    rng->state = 0U;
    rng->inc = lua_pcg_u64_cast(initseq << 1U) | 1U;
    lua_pcg_setseq_64_step_r(rng);
    rng->state = lua_pcg_u64_cast(rng->state + initstate);
    lua_pcg_setseq_64_step_r(rng);
}

static lua_pcg_u64 lua_pcg_advance_lcg_64(lua_pcg_u64 state, lua_pcg_u64 delta, lua_pcg_u64 cur_mult, lua_pcg_u64 cur_plus)
{
    lua_pcg_u64 acc_mult = 1U;
    lua_pcg_u64 acc_plus = 0U;

    lua_pcg_u64 _delta = delta;
    lua_pcg_u64 _cur_mult = cur_mult;
    lua_pcg_u64 _cur_plus = cur_plus;

    while (_delta > 0U)
    {
        if (_delta & 1U)
        {
            acc_mult = lua_pcg_u64_cast(acc_mult * _cur_mult);
            acc_plus = lua_pcg_u64_cast(acc_plus * _cur_mult + _cur_plus);
        }
        _cur_plus = lua_pcg_u64_cast((_cur_mult + 1U) * _cur_plus);
        _cur_mult = lua_pcg_u64_cast(_cur_mult * _cur_mult);
        _delta >>= 1U;
    }

    return lua_pcg_u64_cast(acc_mult * state + acc_plus);
}

#endif

static void lua_pcg_setseq_64_advance_r(struct lua_pcg_state_setseq_64 *rng, lua_pcg_u64 delta)
{
    rng->state = lua_pcg_advance_lcg_64(rng->state, delta, LUA_PCG_DEFAULT_MULTIPLIER_64, rng->inc);
}

static lua_pcg_u32 lua_pcg_setseq_64_xsh_rr_32_boundedrand_r(struct lua_pcg_state_setseq_64 *rng, lua_pcg_u32 bound)
{
    lua_pcg_u32 r;
    lua_pcg_u32 safebound = lua_pcg_u32_cast(bound);
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(push)
#pragma warning(disable:4146)
#endif
    lua_pcg_u32 threshold = (-safebound) % safebound;
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(pop)
#endif

    do
    {
        r = lua_pcg_setseq_64_xsh_rr_32_random_r(rng);
    }
    while (r < threshold);

    return (r % safebound);
}

typedef struct lua_pcg_state_setseq_64 lua_pcg32_random_t;
#define lua_pcg32_random_r lua_pcg_setseq_64_xsh_rr_32_random_r
#define lua_pcg32_srandom_r lua_pcg_setseq_64_srandom_r
#define lua_pcg32_boundedrand_r lua_pcg_setseq_64_xsh_rr_32_boundedrand_r
#define lua_pcg32_advance_r lua_pcg_setseq_64_advance_r

/*
** *****************************************************
** *****************************************************
** 
** Now that 128-bit integer support has been emulated,
** and all the operations needed by pcg64 are available,
** we are going to implement pcg64.
** 
** *****************************************************
** *****************************************************
*/

struct lua_pcg_state_setseq_128
{
    lua_pcg_u128 state;
    lua_pcg_u128 inc;
};

/* 
** LUA_PCG_DEFAULT_MULTIPLIER_128 = 4865540595714422341 | (2549297995355413924 << 64)
**                                = 0x4385DF649FCCF645 | (0x2360ED051FC65DA4 << 0x40)
**                                = (0x9FCCF645 | (0x4385DF64 << 0x20)) | ((0x1FC65DA4 | (0x2360ED05 << 0x20)) << 0x40)
**                                = 0x2360ED051FC65DA44385DF649FCCF645
*/
#define LUA_PCG_DEFAULT_MULTIPLIER_128 (lua_pcg_u128_lh(lua_pcg_u64_lh(0x9FCCF645,0x4385DF64), lua_pcg_u64_lh(0x1FC65DA4,0x2360ED05)))

#ifdef LUA_PCG_U128_EMULATED

static lua_pcg_u64 lua_pcg_rotr_64(lua_pcg_u64 value, unsigned int rot)
{
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(push)
#pragma warning(disable:4146)
#endif
    return (lua_pcg_u64_bor((lua_pcg_u64_rsh(value, rot)), (lua_pcg_u64_lsh(value, ((-rot) & 63)))));
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(pop)
#endif
}

static lua_pcg_u64 lua_pcg_output_xsl_rr_128_64(lua_pcg_u128 state)
{
    /* 
    ** lua_pcg_rotr_64(
    **     ((lua_pcg_u64)( state >> 64U )) ^ ((lua_pcg_u64)state),
    **     (unsigned int)((lua_pcg_u32) ( state >> 122U ) )
    ** )
    ** 
    ** Note: we could simplify it as follows
    ** 
    ** lua_pcg_rotr_64(
    **     state.high ^ state.low,
    **     (unsigned int)((lua_pcg_u32) ( state >> 122U ) )
    ** )
    ** 
    ** However, since we need to trim bits over 64-bits, it is safer
    ** to perform operations with xor's and right-shifts, because
    ** these operations trims bits out of 64-bit range.
    */
    return lua_pcg_rotr_64(
        lua_pcg_u64_xor(lua_pcg_u128_cast_to_u64(lua_pcg_u128_rsh(state, 64U)), lua_pcg_u128_cast_to_u64(state)),
        (unsigned int)lua_pcg_u128_cast_to_u32(lua_pcg_u128_rsh(state, 122U))
    );
}

static void lua_pcg_setseq_128_step_r(struct lua_pcg_state_setseq_128 *rng)
{
    /* rng->state = rng->state * LUA_PCG_DEFAULT_MULTIPLIER_128 + rng->inc */
    rng->state = lua_pcg_u128_sum(lua_pcg_u128_mul(rng->state, LUA_PCG_DEFAULT_MULTIPLIER_128), rng->inc);
}

static lua_pcg_u64 lua_pcg_setseq_128_xsl_rr_64_random_r(struct lua_pcg_state_setseq_128 *rng)
{
    lua_pcg_setseq_128_step_r(rng);
    return lua_pcg_output_xsl_rr_128_64(rng->state);
}

static void lua_pcg_setseq_128_srandom_r(struct lua_pcg_state_setseq_128 *rng, lua_pcg_u128 initstate, lua_pcg_u128 initseq)
{
    /* rng->state = 0U */
    rng->state = lua_pcg_u128_lh(lua_pcg_u64_lh(0U, 0U), lua_pcg_u64_lh(0U, 0U));

    /* rng->inc = (initseq << 1U) | 1U */
    rng->inc = lua_pcg_u128_bor(lua_pcg_u128_lsh(initseq, 1U), lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U)));

    lua_pcg_setseq_128_step_r(rng);

    /* rng->state = rng->state + initstate */
    rng->state = lua_pcg_u128_sum(rng->state, initstate);

    lua_pcg_setseq_128_step_r(rng);
}

static lua_pcg_u64 lua_pcg_setseq_128_xsl_rr_64_boundedrand_r(struct lua_pcg_state_setseq_128 *rng, lua_pcg_u64 bound)
{
    int div_res;
    lua_pcg_u64 boundedrand, q, r, threshold;
    boundedrand = lua_pcg_u64_lh(0U, 0U);
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(push)
#pragma warning(disable:4146)
#endif
    div_res = lua_pcg_u64_div(lua_pcg_u64_additive_inverse(bound), bound, &q, &threshold);
#if (defined(_MSC_VER) && _MSC_VER >= 1100)
#pragma warning(pop)
#endif
    if (div_res == 0)
    {
        do
        {
            r = lua_pcg_setseq_128_xsl_rr_64_random_r(rng);
        }
        while (lua_pcg_u64_lt(r, threshold));

        div_res = lua_pcg_u64_div(r, bound, &q, &boundedrand);
        if (div_res != 0)
        {
            boundedrand = lua_pcg_u64_lh(0U, 0U);
        }
    }
    return boundedrand;
}

static lua_pcg_u128 lua_pcg_advance_lcg_128(lua_pcg_u128 state, lua_pcg_u128 delta, lua_pcg_u128 cur_mult, lua_pcg_u128 cur_plus)
{
    /* 0U */
    lua_pcg_u128 zero = lua_pcg_u128_lh(lua_pcg_u64_lh(0U, 0U), lua_pcg_u64_lh(0U, 0U));
    /* 1U */
    lua_pcg_u128 one = lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U));

    /* acc_mult = 1U */
    lua_pcg_u128 acc_mult = lua_pcg_u128_lh(lua_pcg_u64_lh(1U, 0U), lua_pcg_u64_lh(0U, 0U));

    /* acc_plus = 0U */
    lua_pcg_u128 acc_plus = lua_pcg_u128_lh(lua_pcg_u64_lh(0U, 0U), lua_pcg_u64_lh(0U, 0U));

    lua_pcg_u128 _delta = delta;
    lua_pcg_u128 _cur_mult = cur_mult;
    lua_pcg_u128 _cur_plus = cur_plus;

    /* while (delta > 0) */
    while (lua_pcg_u128_gt(_delta, zero))
    {
        /* if (delta & 1) */
        if (lua_pcg_u128_neq(lua_pcg_u128_band(_delta, one), zero))
        {
            /* acc_mult *= cur_mult */
            acc_mult = lua_pcg_u128_mul(acc_mult, _cur_mult);

            /* acc_plus = acc_plus * cur_mult + cur_plus */
            acc_plus = lua_pcg_u128_sum(lua_pcg_u128_mul(acc_plus, _cur_mult), _cur_plus);
        }

        /* cur_plus = (cur_mult + 1) * cur_plus */
        _cur_plus = lua_pcg_u128_mul(lua_pcg_u128_sum(_cur_mult, one), _cur_plus);

        /* cur_mult *= cur_mult */
        _cur_mult = lua_pcg_u128_mul(_cur_mult, _cur_mult);

        /* delta /= 2 which is equal to delta >>= 1 */
        _delta = lua_pcg_u128_rsh(_delta, 1);
    }

    /* return acc_mult * state + acc_plus */
    return lua_pcg_u128_sum(lua_pcg_u128_mul(acc_mult, state), acc_plus);
}

#else /* both 64-bit and 128-bit are available */

static lua_pcg_u64 lua_pcg_rotr_64(lua_pcg_u64 value, unsigned int rot)
{
    return ((lua_pcg_u64_cast(value) >> rot) | (lua_pcg_u64_cast(value << ((- rot) & 63))));
}

static lua_pcg_u64 lua_pcg_output_xsl_rr_128_64(lua_pcg_u128 state)
{
    return lua_pcg_rotr_64(lua_pcg_u128_cast_to_u64(state >> 64U) ^ lua_pcg_u128_cast_to_u64(state), (unsigned int)(state >> 122U));
}

static void lua_pcg_setseq_128_step_r(struct lua_pcg_state_setseq_128 *rng)
{
    rng->state = rng->state * LUA_PCG_DEFAULT_MULTIPLIER_128 + rng->inc;
}

static lua_pcg_u64 lua_pcg_setseq_128_xsl_rr_64_random_r(struct lua_pcg_state_setseq_128 *rng)
{
    lua_pcg_setseq_128_step_r(rng);
    return lua_pcg_output_xsl_rr_128_64(rng->state);
}

static void lua_pcg_setseq_128_srandom_r(struct lua_pcg_state_setseq_128 *rng, lua_pcg_u128 initstate, lua_pcg_u128 initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1U) | 1U;
    lua_pcg_setseq_128_step_r(rng);
    rng->state = rng->state + initstate;
    lua_pcg_setseq_128_step_r(rng);
}

static lua_pcg_u64 lua_pcg_setseq_128_xsl_rr_64_boundedrand_r(struct lua_pcg_state_setseq_128 *rng, lua_pcg_u64 bound)
{
    lua_pcg_u64 r;
    lua_pcg_u64 safebound = lua_pcg_u64_cast(bound);
    lua_pcg_u64 threshold = (-safebound) % safebound;

    do
    {
        r = lua_pcg_setseq_128_xsl_rr_64_random_r(rng);
    }
    while (r < threshold);

    return (r % safebound);
}

static lua_pcg_u128 lua_pcg_advance_lcg_128(lua_pcg_u128 state, lua_pcg_u128 delta, lua_pcg_u128 cur_mult, lua_pcg_u128 cur_plus)
{
    lua_pcg_u128 acc_mult = 1U;
    lua_pcg_u128 acc_plus = 0U;

    lua_pcg_u128 _delta = delta;
    lua_pcg_u128 _cur_mult = cur_mult;
    lua_pcg_u128 _cur_plus = cur_plus;

    while (_delta > 0U)
    {
        if (_delta & 1U)
        {
            acc_mult = acc_mult * _cur_mult;
            acc_plus = acc_plus * _cur_mult + _cur_plus;
        }
        _cur_plus = (_cur_mult + 1U) * _cur_plus;
        _cur_mult = _cur_mult * _cur_mult;
        _delta >>= 1U;
    }

    return acc_mult * state + acc_plus;
}

#endif

static void lua_pcg_setseq_128_advance_r(struct lua_pcg_state_setseq_128 *rng, lua_pcg_u128 delta)
{
    rng->state = lua_pcg_advance_lcg_128(rng->state, delta, LUA_PCG_DEFAULT_MULTIPLIER_128, rng->inc);
}

typedef struct lua_pcg_state_setseq_128 lua_pcg64_random_t;
#define lua_pcg64_random_r lua_pcg_setseq_128_xsl_rr_64_random_r
#define lua_pcg64_srandom_r lua_pcg_setseq_128_srandom_r
#define lua_pcg64_boundedrand_r lua_pcg_setseq_128_xsl_rr_64_boundedrand_r
#define lua_pcg64_advance_r lua_pcg_setseq_128_advance_r

/*
** 
** Start of utility functions
** to exchange (parse from / push to)
** lua_pcg_u64 and lua_pcg_u128 values
** with Lua scripts.
** 
*/

/* 
** Parses a hex digit matching
** the following regex pattern: [0-9a-fA-F]
** 
** return value:
**    0: everything went fine
**    1: unexpected translation from hex digit to decimal
**    2: not a hex digit
*/
static int lua_pcg_parse_hexdigit(char c, lua_pcg_u32 *digit)
{
    int res = 0;
    int d;

    switch (c)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            d = ((int)c) - ((int)'0');

            if (!(0 <= d && d <= 9))
            {
                res = 1;
            }

            break;
        }
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        {
            d = ((int)c) - ((int)'a') + 0xA;

            if (!(0xA <= d && d <= 0xF))
            {
                res = 1;
            }

            break;
        }
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        {
            d = ((int)c) - ((int)'A') + 0xA;

            if (!(0xA <= d && d <= 0xF))
            {
                res = 1;
            }

            break;
        }
        default:
        {
            res = 2;
            break;
        }
    }

    if (res == 0)
    {
        *digit = (lua_pcg_u32)d;
    }

    return res;
}

/* 
** Parses a lua_pcg_u32 from a hex string (right to left)
** starting at the (e)nd position
** but not going further than (s)tart position.
** Matches the following regex pattern: [0-9a-fA-F]{1,8}
** 
** return value:
**    0: everything went fine
**    1: unexpected translation from hex digit to decimal
**    2: not a hex digit
**    3: s or e are out of range
*/
static int lua_pcg_parse_u32_from_hexstring(const char *buff, int s, int e, lua_pcg_u32 *result, lua_pcg_u32 *consumed, int *failed_index)
{
    int res = 0;
    lua_pcg_u32 digit;
    lua_pcg_u32 value = 0U;
    lua_pcg_u32 con = 0U;
    int i = e;

    if (e < 0 || e < s)
    {
        res = 3;
    }
    else
    {
        while ((i >= s) && (con < 8U) && ((res = lua_pcg_parse_hexdigit(buff[i], &digit)) == 0))
        {
            con++;

            if (con == 1U)
            {
                value |= digit;
            }
            else /* con <= 8U */
            {
                value |= (digit << ((con - 1U) << 2U));
            }

            i--;
        }
    }

    if (res == 0)
    {
        *result = value;
        *consumed = con;
    }
    else
    {
        *failed_index = i;
    }

    return res;
}

/*
** Parses lua_pcg_u64 as hex string
** located at stack position given by 'index'
** 
** Note: the hex string must have a value
** on the interval [0x0, 0xFFFFFFFFFFFFFFFF]
*/
static lua_pcg_u64 lua_pcg_parse_u64_hex_arg(lua_State *L, int index)
{
    int i, j;
    size_t len;
    lua_pcg_u32 consumed = 0U;

    /* holds low and high */
    lua_pcg_u32 words[2];

    int parse_status;
    int failed_index = -1;
    const char *s = luaL_checklstring(L, index, &len);

    luaL_argcheck(L, len >= 2, index, "Hex string too short.");
    luaL_argcheck(L, len <= 18, index, "Too many characters in the hex string.");
    luaL_argcheck(L, (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')), index, "Hex prefix not found.");

    /* fill low and high with zeros */
    memset((void *)words, 0, sizeof(words));

    i = (int)(len - 1);
    j = 0;

    while (i >= 2)
    {
        parse_status = lua_pcg_parse_u32_from_hexstring(s, 2, i, &(words[j]), &consumed, &failed_index);

        switch (parse_status)
        {
            case 0:
            {
                /* everything went fine */
                break;
            }
            case 1:
            {
                luaL_error(L, "Unxpected condition while parsing digit at position %d on argument #%d", failed_index + 1, index);
                break;
            }
            case 2:
            {
                luaL_error(L, "Position %d is not a digit on argument #%d", failed_index + 1, index);
                break;
            }
            case 3:
            {
                luaL_error(L, "Internal error (wrong string positions [%d, %d]). Report this issue to the team maintaining the lua-pcg library.", 2, i);
                break;
            }
            default:
            {
                luaL_error(L, "Unknown digit conversion result at position %d on argument #%d", failed_index + 1, index);
                break;
            }
        }

        i -= consumed;
        j++;
    }

    return lua_pcg_u64_lh(words[0], words[1]);
}

/*
** Parses lua_pcg_u64 as table argument
** located at stack position given by 'index'
** 
** Note: at most, 8 bytes are read from
** the table argument.
*/
static lua_pcg_u64 lua_pcg_parse_u64_table_arg(lua_State *L, int index)
{
    size_t i;
    size_t table_size;
    lua_Integer n;
    lua_pcg_u64 element;
    unsigned int bits_to_shift;
    lua_pcg_u64 result = lua_pcg_u64_lh(0U, 0U);

    luaL_checktype(L, index, LUA_TTABLE);

    table_size = lua_pcg_table_length(L, index);

    luaL_argcheck(L, table_size > 0U, index, "empty table is not allowed");

    if (table_size > 8U)
    {
        table_size = 8U;
    }

    /* places a copy of the table on the top */
    lua_pushvalue(L, index);

    for (i = 1, bits_to_shift = 0U; i <= table_size; i++, bits_to_shift += 8U)
    {
        lua_pushinteger(L, (lua_Integer)i);
        lua_gettable(L, -2);

        if (lua_pcg_aux_isinteger(L, -1))
        {
            n = lua_tointeger(L, -1);
            if (!(0 <= n && n <= 0xFF))
            {
                luaL_error(L, "integer is out of range at table position [%d]", (int)i);
            }

            if (i == 1)
            {
                element = lua_pcg_u64_lh((lua_pcg_u32)n, 0U);
            }
            else
            {
                element = lua_pcg_u64_lsh(lua_pcg_u64_lh((lua_pcg_u32)n, 0U), bits_to_shift);
            }

            result = lua_pcg_u64_bor(result, element);

            /* removes the byte from the top */
            lua_pop(L, 1);
        }
        else
        {
            luaL_error(L, "integer expected at table position [%d]", (int)i);
        }
    }

    /* removes the copy of the table from the top */
    lua_pop(L, 1);

    return result;
}

/*
** Parses lua_pcg_u64 argument
** located at stack position given by 'index'
*/
static lua_pcg_u64 lua_pcg_parse_u64_arg(lua_State *L, int index)
{
    lua_pcg_u64 result;
    int t = lua_type(L, index);

    switch (t)
    {
        case LUA_TTABLE:
        {
            result = lua_pcg_parse_u64_table_arg(L, index);
            break;
        }
        case LUA_TSTRING:
        {
            result = lua_pcg_parse_u64_hex_arg(L, index);
            break;
        }
        default:
        {
            luaL_error(L, "Invalid argument type");

            /* 
            ** This line will never be hit.
            ** It is here just let the compiler happy
            */
            result = lua_pcg_u64_lh(0U, 0U);
            break;
        }
    }

    return result;
}

/*
** Parses lua_pcg_u128 as hex string
** located at stack position given by 'index'
** 
** Note: the hex string must have a value
** on the interval [0x0, 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF]
*/
static lua_pcg_u128 lua_pcg_parse_u128_hex_arg(lua_State *L, int index)
{
    int i, j;
    size_t len;
    lua_pcg_u32 consumed = 0U;

    /* holds low (low and high) and high (low and high) */
    lua_pcg_u32 words[4];

    int parse_status;
    int failed_index = -1;
    const char *s = luaL_checklstring(L, index, &len);

    luaL_argcheck(L, len >= 2, index, "Hex string too short.");
    luaL_argcheck(L, len <= 34, index, "Too many characters in the hex string.");
    luaL_argcheck(L, (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')), index, "Hex prefix not found.");

    /* fill low and high with zeros */
    memset((void *)words, 0, sizeof(words));

    i = (int)(len - 1);
    j = 0;

    while (i >= 2)
    {
        parse_status = lua_pcg_parse_u32_from_hexstring(s, 2, i, &(words[j]), &consumed, &failed_index);

        switch (parse_status)
        {
            case 0:
            {
                /* everything went fine */
                break;
            }
            case 1:
            {
                luaL_error(L, "Unxpected condition while parsing digit at position %d on argument #%d", failed_index + 1, index);
                break;
            }
            case 2:
            {
                luaL_error(L, "Position %d is not a digit on argument #%d", failed_index + 1, index);
                break;
            }
            case 3:
            {
                luaL_error(L, "Internal error (wrong string positions [%d, %d]). Report this issue to the team maintaining the lua-pcg library.", 2, i);
                break;
            }
            default:
            {
                luaL_error(L, "Unknown digit conversion result at position %d on argument #%d", failed_index + 1, index);
                break;
            }
        }

        i -= consumed;
        j++;
    }

    return lua_pcg_u128_lh(lua_pcg_u64_lh(words[0], words[1]), lua_pcg_u64_lh(words[2], words[3]));
}

/*
** Parses lua_pcg_u128 as table argument
** located at stack position given by 'index'
** 
** Note: at most, 16 bytes are read from
** the table argument.
*/
static lua_pcg_u128 lua_pcg_parse_u128_table_arg(lua_State *L, int index)
{
    size_t i;
    size_t table_size;
    lua_Integer n;
    lua_pcg_u128 element;
    unsigned int bits_to_shift;
    lua_pcg_u128 result = lua_pcg_u128_lh(lua_pcg_u64_lh(0U, 0U), lua_pcg_u64_lh(0U, 0U));

    luaL_checktype(L, index, LUA_TTABLE);

    table_size = lua_pcg_table_length(L, index);

    luaL_argcheck(L, table_size > 0U, index, "empty table is not allowed");

    if (table_size > 16U)
    {
        table_size = 16U;
    }

    /* places a copy of the table on the top */
    lua_pushvalue(L, index);

    for (i = 1, bits_to_shift = 0U; i <= table_size; i++, bits_to_shift += 8U)
    {
        lua_pushinteger(L, (lua_Integer)i);
        lua_gettable(L, -2);

        if (lua_pcg_aux_isinteger(L, -1))
        {
            n = lua_tointeger(L, -1);
            if (!(0 <= n && n <= 0xFF))
            {
                luaL_error(L, "integer is out of range at table position [%d]", (int)i);
            }

            if (i == 1)
            {
                element = lua_pcg_u128_lh(lua_pcg_u64_lh((lua_pcg_u32)n, 0U), lua_pcg_u64_lh(0U, 0U));
            }
            else
            {
                element = lua_pcg_u128_lsh(lua_pcg_u128_lh(lua_pcg_u64_lh((lua_pcg_u32)n, 0U), lua_pcg_u64_lh(0U, 0U)), bits_to_shift);
            }

            result = lua_pcg_u128_bor(result, element);

            /* removes the byte from the top */
            lua_pop(L, 1);
        }
        else
        {
            luaL_error(L, "integer expected at table position [%d]", (int)i);
        }
    }

    /* removes the copy of the table from the top */
    lua_pop(L, 1);

    return result;
}

/*
** Parses lua_pcg_u128 argument
** located at stack position given by 'index'
*/
static lua_pcg_u128 lua_pcg_parse_u128_arg(lua_State *L, int index)
{
    lua_pcg_u128 result;
    int t = lua_type(L, index);

    switch (t)
    {
        case LUA_TTABLE:
        {
            result = lua_pcg_parse_u128_table_arg(L, index);
            break;
        }
        case LUA_TSTRING:
        {
            result = lua_pcg_parse_u128_hex_arg(L, index);
            break;
        }
        default:
        {
            luaL_error(L, "Invalid argument type");

            /* 
            ** This line will never be hit.
            ** It is here just let the compiler happy
            */
            result = lua_pcg_u128_lh(lua_pcg_u64_lh(0U, 0U), lua_pcg_u64_lh(0U, 0U));
            break;
        }
    }

    return result;
}

#ifdef LUA_PCG_DEBUGGING
/* pushes a hex string representing lua_pcg_u64 on Lua stack
** 
** Note: this method is mainly for debugging purposes
*/
static int lua_pcg_push_lua_pcg_u64(lua_State *L, lua_pcg_u64 value, int lower)
{
    int i;
    lua_pcg_u8 byte;
    lua_pcg_u64 current;
    char buff[2 + 8 * 2 + 1];

    buff[0] = '0';
    buff[1] = lower ? 'x' : 'X';
    buff[(sizeof(buff) / sizeof(char)) - 1] = '\0';
    memcpy((void *)&current, (const void *)&value, sizeof(lua_pcg_u64));

    for (i = (sizeof(buff) / sizeof(char)) - 2; i >= 2; i--, current = lua_pcg_u64_rsh(current, 4U))
    {
        byte = (lua_pcg_u64_cast_to_u8(current) & 0xF);

        if (0xA <= byte && byte <= 0xF)
        {
            buff[i] = (char)((lower ? 'a' : 'A') + ((char)(byte - 0xA)));
        }
        else
        {
            buff[i] = (char)('0' + ((char)byte));
        }
    }

    lua_pushstring(L, buff);

    return 1;
}

/* pushes a hex string representing lua_pcg_u128 on Lua stack 
** 
** Note: this method is mainly for debugging purposes
*/
static int lua_pcg_push_lua_pcg_u128(lua_State *L, lua_pcg_u128 value, int lower)
{
    int i;
    lua_pcg_u8 byte;
    lua_pcg_u128 current;
    char buff[2 + 16 * 2 + 1];

    buff[0] = '0';
    buff[1] = lower ? 'x' : 'X';
    buff[(sizeof(buff) / sizeof(char)) - 1] = '\0';
    memcpy((void *)&current, (const void *)&value, sizeof(lua_pcg_u128));

    for (i = (sizeof(buff) / sizeof(char)) - 2; i >= 2; i--, current = lua_pcg_u128_rsh(current, 4U))
    {
        byte = (lua_pcg_u128_cast_to_u8(current) & 0xF);

        if (0xA <= byte && byte <= 0xF)
        {
            buff[i] = (char)((lower ? 'a' : 'A') + ((char)(byte - 0xA)));
        }
        else
        {
            buff[i] = (char)('0' + ((char)byte));
        }
    }

    lua_pushstring(L, buff);

    return 1;
}
#endif

/* Permutes the bytes on buffer,
** and sets a lua_pcg_u32 on values
** using the first 4 bytes on buffer.
** 
** Note 1: buffer is supposed
**         to hold at least 4 bytes
** 
** Note 2: at most, 8 different permutations
**         are calculated, based on the fact that
**         buffer has at least 4 bytes. Through
**         this strategy, we are able to generate 
**         two lua_pcg_u128 integers, because
**         8 permutations * 4 bytes = 32 bytes ( = 2 * 128 bits)
** 
** Note 3: this function used to permute array
***        elements was adapted from Lua PIL:
**         ( https://www.lua.org/pil/9.3.html )
*/
static void lua_pcg_permute_bytes(lua_pcg_u8 *buffer, int n, lua_pcg_u32 *values, int *count)
{
    int i;
    int last_pos;
    lua_pcg_u8 aux;

    if (*count < 8)
    {
        if (n == 0)
        {
            values[*count] = (
                ((lua_pcg_u32)(0xFF & buffer[0])) |
                (((lua_pcg_u32)(0xFF & buffer[1])) << 8) |
                (((lua_pcg_u32)(0xFF & buffer[2])) << 16) |
                (((lua_pcg_u32)(0xFF & buffer[3])) << 24)
            );
            *count = (*count) + 1;
        }
        else
        {
            last_pos = n - 1;
            for (i = 0; *count < 8 && i < n; i++)
            {
                /* swap last_pos element with the one at i-th position */
                aux = buffer[last_pos];
                buffer[last_pos] = buffer[i];
                buffer[i] = aux;

                lua_pcg_permute_bytes(buffer, last_pos, values, count);

                /* restore changed positions */
                aux = buffer[last_pos];
                buffer[last_pos] = buffer[i];
                buffer[i] = aux;
            }
        }
    }
}

/* 
** Basic (rudimentary) random bytes generator based on:
**   1) dynamic address of rng state (ud parameter)
**   2) current time
** 
** Note 1: parameter ud cannot be NULL.
** Note 2: parameters v1 and v2 might be NULL.
*/
static void lua_pcg_fill_with_random_bytes(void *ud, lua_pcg_u128 *v1, lua_pcg_u128 *v2)
{
    int count;
    lua_pcg_u32 values[8];
    lua_pcg_u8 bytes[sizeof(lua_pcg_u32)];
    lua_pcg_u32 now32 = (lua_pcg_u32)(time(NULL));
    lua_pcg_u32 now = lua_pcg_u32_cast(now32);
    lua_pcg_u32 ud_address = lua_pcg_u32_cast((size_t)ud);
    lua_pcg_u32 value = ud_address ^ now;

    memset(bytes, 0, sizeof(bytes));
    if (sizeof(bytes) < sizeof(lua_pcg_u32))
    {
        memcpy(bytes, &value, sizeof(bytes));
    }
    else
    {
        memcpy(bytes, &value, sizeof(lua_pcg_u32));
    }

    /* count must be initialized with 0 */
    count = 0;
    lua_pcg_permute_bytes(bytes, 4, values, &count);

    if (v1 != NULL)
    {
        *v1 = lua_pcg_u128_lh(
            lua_pcg_u64_lh(values[0], values[1]),
            lua_pcg_u64_lh(values[2], values[3])
        );
    }

    if (v2 != NULL)
    {
        *v2 = lua_pcg_u128_lh(
            lua_pcg_u64_lh(values[4], values[5]),
            lua_pcg_u64_lh(values[6], values[7])
        );
    }
}

/*
** Dynamically allocate 'size' bytes
*/
static void *lua_pcg_alloc(lua_State *L, size_t size)
{
#ifdef LUA_PCG_USE_LUA_ALLOC
    void *ud;
    lua_Alloc allocf = lua_getallocf(L, &ud);
    return allocf(ud, NULL, 0, size);
#else
    (void)L;
    return malloc(size);
#endif
}

/*
** Free the memory on 'ptr' which is holding 'size' bytes
** previously allocated.
*/
static void lua_pcg_free(lua_State *L, void *ptr, size_t size)
{
#ifdef LUA_PCG_USE_LUA_ALLOC
    void *ud;
    lua_Alloc allocf = lua_getallocf(L, &ud);
    ((void)allocf(ud, ptr, size, 0));
#else
    (void)L;
    (void)size;
    return free(ptr);
#endif
}

/* end of utility functions */

/* 
** *********************************************
** *********************************************
** 
** Now that pcg32 methods were implemented,
** we are going to bridge it to Lua.
** 
** *********************************************
** *********************************************
*/

/*
** Structure to wrap a lua_pcg32_random_t
** 
** Note: it might be thought unnecessary
**       to wrap the type 'lua_pcg32_random_t'
**       in order to expose methods to Lua.
**       However, 'lua_newuserdata' has
**       some hard to spot alignment problems
**       (see https://github.com/LuaJIT/LuaJIT/issues/1161 ).
** 
**       Even with PUC-Lua interpreter, on Linux with GCC, I've ran
**       into such alignment issues, especially
**       in the 'advance' function on lua_pcg64_random_t
**       below for the (native system's provided) __int128.
** 
**       Thus, to avoid any possibility to run into
**       such alignment issues, we wrap the
**       'lua_pcg32_random_t' and allocate it
**       using 'malloc' which works well in all cases.
*/
typedef struct
{
    lua_pcg32_random_t *rng;
} lua_pcg32_random_t_wrapper;

static lua_pcg32_random_t_wrapper *lua_pcg_pcg32_check(lua_State *L, int index)
{
    void *ud = luaL_checkudata(L, index, LUA_PCG_PCG32_METATABLE);
    luaL_argcheck(L, ud != NULL, index, "pcg32 random expected");
    return (lua_pcg32_random_t_wrapper *)ud;
}

static lua_pcg32_random_t *lua_pcg_pcg32_check_rng(lua_State *L, int index)
{
    lua_pcg32_random_t_wrapper *wrapper = lua_pcg_pcg32_check(L, index);
    luaL_argcheck(L, wrapper->rng != NULL, index, "pcg32 random was closed previously");
    return wrapper->rng;
}

/* creates a lua_pcg32_random_t (rng) */
static int lua_pcg_pcg32_new(lua_State *L)
{
    lua_pcg_u128 v1;
    lua_pcg_u128 v2;
    lua_pcg_u64 initstate;
    lua_pcg_u64 initseq;
    lua_pcg32_random_t_wrapper *wrapper;
    lua_pcg32_random_t *rng;
    void *ud;
    int nargs;

    nargs = lua_gettop(L);
    ud = lua_newuserdata(L, sizeof(lua_pcg32_random_t_wrapper));
    if (ud == NULL)
    {
        return luaL_error(L, "Memory allocation on Lua failed to create user data for pcg32 random");
    }
    luaL_getmetatable(L, LUA_PCG_PCG32_METATABLE);
    lua_setmetatable(L, -2);

    wrapper = (lua_pcg32_random_t_wrapper *)ud;
    wrapper->rng = (lua_pcg32_random_t *)(lua_pcg_alloc(L, sizeof(lua_pcg32_random_t)));
    if (wrapper->rng == NULL)
    {
        return luaL_error(L, "Memory allocation failed to initialize lua_pcg32_random_t");
    }
    rng = wrapper->rng;

    switch (nargs)
    {
        case 0: /* no args? generate both initstate and initseq */
        {
            lua_pcg_fill_with_random_bytes(ud, &v1, &v2);
            initstate = lua_pcg_u128_cast_to_u64(v1);
            initseq = lua_pcg_u128_cast_to_u64(v2);
            break;
        }
        case 1: /* only initstate was provided? then generate initseq */
        {
            initstate = lua_pcg_parse_u64_arg(L, 1);
            lua_pcg_fill_with_random_bytes(ud, NULL, &v2);
            initseq = lua_pcg_u128_cast_to_u64(v2);
            break;
        }
        case 2: /* both initstate and initseq were provided */
        {
            initstate = lua_pcg_parse_u64_arg(L, 1);
            initseq = lua_pcg_parse_u64_arg(L, 2);
            break;
        }
        default:
        {
            return luaL_error(L, "Unknown number of arguments to create a pcg32 instance");
        }
    }

    lua_pcg32_srandom_r(rng, initstate, initseq);
    return 1;
}

/* advances a lua_pcg32_random_t (rng) by delta */
static int lua_pcg_pcg32_advance(lua_State *L)
{
    lua_pcg32_random_t *rng = lua_pcg_pcg32_check_rng(L, 1);
    lua_pcg_u64 delta = lua_pcg_parse_u64_arg(L, 2);
    lua_pcg32_advance_r(rng, delta);
    return 0;
}

/* seed the pcg32 rng */
static int lua_pcg_pcg32_seed(lua_State *L)
{
    lua_pcg32_random_t *rng = lua_pcg_pcg32_check_rng(L, 1);
    lua_pcg_u64 initstate = lua_pcg_parse_u64_arg(L, 2);
    lua_pcg_u64 initseq = lua_pcg_parse_u64_arg(L, 3);
    lua_pcg32_srandom_r(rng, initstate, initseq);
    return 0;
}

/* gets the next lua_pcg_u32 value from the pcg32 rng */
static int lua_pcg_pcg32_next(lua_State *L)
{
    lua_pcg_u32 n;
    lua_pcg_u32 bound;
    lua_Integer a, b, c;
    int nargs = lua_gettop(L);
    lua_pcg32_random_t *rng = lua_pcg_pcg32_check_rng(L, 1);

    nargs--;

    switch (nargs)
    {
        case 0: /* no args? */
        {
            n = lua_pcg32_random_r(rng);
            lua_pushinteger(L, (lua_Integer)n);
            break;
        }
        case 1: /* bound is given */
        {
            a = luaL_checkinteger(L, 2);
            luaL_argcheck(L, 0 < a && a <= 0xFFFFFFFF, 2, "bound is out of [1, 4294967295] range");
            bound = lua_pcg_u32_cast(a);

            n = lua_pcg32_boundedrand_r(rng, bound);
            lua_pushinteger(L, (lua_Integer)n);
            break;
        }
        case 2: /* a, b were provided */
        {
            a = luaL_checkinteger(L, 2);
            b = luaL_checkinteger(L, 3);
            luaL_argcheck(L, a < b, 2, "a cannot be greater than or equal to b");

            c = b - a;
            luaL_argcheck(L, c <= 0xFFFFFFFF, 2, "the integer (b - a) is out of [1, 4294967295] range");
            bound = lua_pcg_u32_cast(c);

            n = lua_pcg32_boundedrand_r(rng, bound);
            lua_pushinteger(L, a + ((lua_Integer)n));
            break;
        }
        default:
        {
            return luaL_error(L, "Unknown number of arguments to generate a next number from pcg32");
        }
    }
    return 1;
}

/* gets the bytes from the next lua_pcg_u32 value provided by the pcg32 rng */
static int lua_pcg_pcg32_nextbytes(lua_State *L)
{
    int i;
    lua_pcg32_random_t *rng = lua_pcg_pcg32_check_rng(L, 1);
    lua_pcg_u32 n = lua_pcg32_random_r(rng);

    lua_createtable(L, 0, 0);

    for (i = 0; i < 4; i++, n >>= 8U)
    {
        lua_pushinteger(L, i + 1);
        lua_pushinteger(L, lua_pcg_u8_cast(n));
        lua_settable(L, -3);
    }

    return 1;
}

/* sets the pcg32 object instance as read-only */
static int lua_pcg_pcg32_newindex(lua_State *L)
{
    return luaL_error(L, "Read-only object");
}

/* frees memory used by each pcg32 random instance */
static int lua_pcg_pcg32_close(lua_State *L)
{
    lua_pcg32_random_t_wrapper *wrapper = lua_pcg_pcg32_check(L, 1);
    if (wrapper->rng != NULL)
    {
        lua_pcg_free(L, wrapper->rng, sizeof(lua_pcg32_random_t));
        wrapper->rng = NULL;
    }
    return 0;
}

static const luaL_Reg lua_pcg_pcg32_funcs[] = {
    {"__gc", lua_pcg_pcg32_close},
    {"advance", lua_pcg_pcg32_advance},
    {"close", lua_pcg_pcg32_close},
    {"new", lua_pcg_pcg32_new},
    {"next", lua_pcg_pcg32_next},
    {"nextbytes", lua_pcg_pcg32_nextbytes},
    {"seed", lua_pcg_pcg32_seed},
    {NULL, NULL}
};
/* end of pcg32 implementation */

/* 
** *********************************************
** *********************************************
** 
** Now that pcg64 methods were implemented,
** we are going to bridge it to Lua.
** 
** *********************************************
** *********************************************
*/

/*
** Structure to wrap a lua_pcg64_random_t
** 
** Note: it might be thought unnecessary
**       to wrap the type 'lua_pcg64_random_t'
**       in order to expose methods to Lua.
**       However, 'lua_newuserdata' has
**       some hard to spot alignment problems
**       (see https://github.com/LuaJIT/LuaJIT/issues/1161 ).
** 
**       Even with PUC-Lua interpreter, on Linux with GCC, I've ran
**       into such alignment issues, especially
**       in the 'advance' function on lua_pcg64_random_t
**       below for the (native system's provided) __int128.
** 
**       Thus, to avoid any possibility to run into
**       such alignment issues, we wrap the
**       'lua_pcg64_random_t' and allocate it
**       using 'malloc' which works well in all cases.
*/
typedef struct
{
    lua_pcg64_random_t *rng;
} lua_pcg64_random_t_wrapper;

static lua_pcg64_random_t_wrapper *lua_pcg_pcg64_check(lua_State *L, int index)
{
    void *ud = luaL_checkudata(L, index, LUA_PCG_PCG64_METATABLE);
    luaL_argcheck(L, ud != NULL, index, "pcg64 random expected");
    return (lua_pcg64_random_t_wrapper *)ud;
}

static lua_pcg64_random_t *lua_pcg_pcg64_check_rng(lua_State *L, int index)
{
    lua_pcg64_random_t_wrapper *wrapper = lua_pcg_pcg64_check(L, index);
    luaL_argcheck(L, wrapper->rng != NULL, index, "pcg64 random was closed previously");
    return wrapper->rng;
}

/* creates a lua_pcg64_random_t (rng) */
static int lua_pcg_pcg64_new(lua_State *L)
{
    lua_pcg_u128 initstate;
    lua_pcg_u128 initseq;
    lua_pcg64_random_t_wrapper *wrapper;
    lua_pcg64_random_t *rng;
    void *ud;
    int nargs;

    nargs = lua_gettop(L);
    ud = lua_newuserdata(L, sizeof(lua_pcg64_random_t_wrapper));
    if (ud == NULL)
    {
        return luaL_error(L, "Memory allocation on Lua failed to create user data for pcg64 random");
    }
    luaL_getmetatable(L, LUA_PCG_PCG64_METATABLE);
    lua_setmetatable(L, -2);

    wrapper = (lua_pcg64_random_t_wrapper *)ud;
    wrapper->rng = (lua_pcg64_random_t *)(lua_pcg_alloc(L, sizeof(lua_pcg64_random_t)));
    if (wrapper->rng == NULL)
    {
        return luaL_error(L, "Memory allocation failed to initialize lua_pcg64_random_t");
    }
    rng = wrapper->rng;

    switch (nargs)
    {
        case 0: /* no args? generate both initstate and initseq */
        {
            lua_pcg_fill_with_random_bytes(ud, &initstate, &initseq);
            break;
        }
        case 1: /* only initstate was provided? then generate initseq */
        {
            initstate = lua_pcg_parse_u128_arg(L, 1);
            lua_pcg_fill_with_random_bytes(ud, NULL, &initseq);
            break;
        }
        case 2: /* both initstate and initseq were provided */
        {
            initstate = lua_pcg_parse_u128_arg(L, 1);
            initseq = lua_pcg_parse_u128_arg(L, 2);
            break;
        }
        default:
        {
            return luaL_error(L, "Unknown number of arguments to create a pcg64 instance.");
        }
    }

    lua_pcg64_srandom_r(rng, initstate, initseq);

    return 1;
}

/* advances a lua_pcg64_random_t (rng) by delta */
static int lua_pcg_pcg64_advance(lua_State *L)
{
    lua_pcg64_random_t *rng = lua_pcg_pcg64_check_rng(L, 1);
    lua_pcg_u128 delta = lua_pcg_parse_u128_arg(L, 2);
    lua_pcg64_advance_r(rng, delta);
    return 0;
}

/* seed the pcg64 rng */
static int lua_pcg_pcg64_seed(lua_State *L)
{
    lua_pcg64_random_t *rng = lua_pcg_pcg64_check_rng(L, 1);
    lua_pcg_u128 initstate = lua_pcg_parse_u128_arg(L, 2);
    lua_pcg_u128 initseq = lua_pcg_parse_u128_arg(L, 3);
    lua_pcg64_srandom_r(rng, initstate, initseq);
    return 0;
}

/* gets the next lua_pcg_u64 value from the pcg64 rng */
static int lua_pcg_pcg64_next(lua_State *L)
{
    lua_pcg_u64 n;
    lua_pcg_u64 bound;
    lua_Integer a, b, c;
    int nargs = lua_gettop(L);
    lua_pcg64_random_t *rng = lua_pcg_pcg64_check_rng(L, 1);

#ifdef LUA_PCG_U64_EMULATED
    int lua_Integer_has_64bit = lua_pcg_lua_Integer_has_64bit();
#endif

    nargs--;

    switch (nargs)
    {
        case 0: /* no args? */
        {
            n = lua_pcg64_random_r(rng);
#ifdef LUA_PCG_U64_EMULATED
            if (lua_Integer_has_64bit) /* assume lua_Integer can hold 64-bit values */
            {
                lua_pushinteger(L, (((lua_Integer)n.low) | (((((lua_Integer)n.high) << 15) << 15) << 2)));
            }
            else
            {
                lua_pushinteger(L, (lua_Integer)(lua_pcg_u64_cast_to_u32(n)));
            }
#else
            lua_pushinteger(L, (lua_Integer)n);
#endif
            break;
        }
        case 1: /* bound is given */
        {
            a = luaL_checkinteger(L, 2);
            luaL_argcheck(L, 0 < a, 2, "bound must be a positive integer");

            if (a <= 0xFFFFFFFF)
            {
                bound = lua_pcg_u64_lh(a, 0);
            }
            else
            {
                bound = lua_pcg_u64_lh(lua_pcg_u32_cast(a), lua_pcg_u32_cast((((a >> 15) >> 15) >> 2)));
            }

            n = lua_pcg64_boundedrand_r(rng, bound);
#ifdef LUA_PCG_U64_EMULATED
            if (lua_Integer_has_64bit)
            {
                lua_pushinteger(L, (((lua_Integer)n.low) | (((((lua_Integer)n.high) << 15) << 15) << 2)));
            }
            else
            {
                lua_pushinteger(L, (lua_Integer)(lua_pcg_u64_cast_to_u32(n)));
            }
#else
            lua_pushinteger(L, (lua_Integer)n);
#endif
            break;
        }
        case 2: /* a, b were provided */
        {
            a = luaL_checkinteger(L, 2);
            b = luaL_checkinteger(L, 3);
            luaL_argcheck(L, a < b, 2, "a cannot be greater than or equal to b");

            c = b - a;
            if (c <= 0xFFFFFFFF)
            {
                bound = lua_pcg_u64_lh(c, 0);
            }
            else
            {
                bound = lua_pcg_u64_lh(lua_pcg_u32_cast(c), lua_pcg_u32_cast((((c >> 15) >> 15) >> 2)));
            }

            n = lua_pcg64_boundedrand_r(rng, bound);
#ifdef LUA_PCG_U64_EMULATED
            if (lua_Integer_has_64bit)
            {
                lua_pushinteger(L, a + (((lua_Integer)n.low) | (((((lua_Integer)n.high) << 15) << 15) << 2)));
            }
            else
            {
                lua_pushinteger(L, a + (lua_Integer)(lua_pcg_u64_cast_to_u32(n)));
            }
#else
            lua_pushinteger(L, a + ((lua_Integer)n));
#endif
            break;
        }
        default:
        {
            return luaL_error(L, "Unknown number of arguments to generate a next number from pcg64");
        }
    }
    return 1;
}

/* gets the bytes from the next lua_pcg_u64 value provided by the pcg64 rng */
static int lua_pcg_pcg64_nextbytes(lua_State *L)
{
    lua_Integer i;
    lua_pcg64_random_t *rng = lua_pcg_pcg64_check_rng(L, 1);
    lua_pcg_u64 n = lua_pcg64_random_r(rng);

    lua_createtable(L, 0, 0);

    for (i = 1; i <= 8; i++, n = lua_pcg_u64_rsh(n, 8U))
    {
        lua_pushinteger(L, i);
        lua_pushinteger(L, lua_pcg_u64_cast_to_u8(n));
        lua_settable(L, -3);
    }

    return 1;
}

/* sets the pcg64 object instance as read-only */
static int lua_pcg_pcg64_newindex(lua_State *L)
{
    return luaL_error(L, "Read-only object");
}

/* frees memory used by each pcg64 random instance */
static int lua_pcg_pcg64_close(lua_State *L)
{
    lua_pcg64_random_t_wrapper *wrapper = lua_pcg_pcg64_check(L, 1);
    if (wrapper->rng != NULL)
    {
        lua_pcg_free(L, wrapper->rng, sizeof(lua_pcg64_random_t));
        wrapper->rng = NULL;
    }
    return 0;
}

static const luaL_Reg lua_pcg_pcg64_funcs[] = {
    {"__gc", lua_pcg_pcg64_close},
    {"advance", lua_pcg_pcg64_advance},
    {"close", lua_pcg_pcg64_close},
    {"new", lua_pcg_pcg64_new},
    {"next", lua_pcg_pcg64_next},
    {"nextbytes", lua_pcg_pcg64_nextbytes},
    {"seed", lua_pcg_pcg64_seed},
    {NULL, NULL}
};
/* end of pcg64 implementation */

/* sets the pcg library as read-only */
static int lua_pcg_newindex(lua_State *L)
{
    return luaL_error(L, "Read-only object");
}

LUA_PCG_EXPORT int luaopen_pcg(lua_State *L)
{
    lua_createtable(L, 0, 0);
    luaL_newmetatable(L, LUA_PCG_METATABLE);

    /* start of pcg32 */
    lua_pushstring(L, "pcg32");
    lua_createtable(L, 0, 0);
    luaL_newmetatable(L, LUA_PCG_PCG32_METATABLE);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, lua_pcg_pcg32_funcs);
#else
    luaL_setfuncs(L, lua_pcg_pcg32_funcs, 0);
#endif

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lua_pcg_pcg32_newindex);
    lua_settable(L, -3);

    lua_pushstring(L, "__metatable");
    lua_pushboolean(L, 0);
    lua_settable(L, -3);

    lua_setmetatable(L, -2);
    lua_settable(L, -3);
    /* end of pcg32 */

    /* start of pcg64 */
    lua_pushstring(L, "pcg64");
    lua_createtable(L, 0, 0);
    luaL_newmetatable(L, LUA_PCG_PCG64_METATABLE);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, lua_pcg_pcg64_funcs);
#else
    luaL_setfuncs(L, lua_pcg_pcg64_funcs, 0);
#endif

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lua_pcg_pcg64_newindex);
    lua_settable(L, -3);

    lua_pushstring(L, "__metatable");
    lua_pushboolean(L, 0);
    lua_settable(L, -3);

    lua_setmetatable(L, -2);
    lua_settable(L, -3);
    /* end of pcg64 */

    lua_pushstring(L, "version");
    lua_pushstring(L, LUA_PCG_VERSION);
    lua_settable(L, -3);

    lua_pushstring(L, "emulation128bit");
#ifdef LUA_PCG_U128_EMULATED
    lua_pushboolean(L, 1);
#else
    lua_pushboolean(L, 0);
#endif
    lua_settable(L, -3);

    lua_pushstring(L, "emulation64bit");
#ifdef LUA_PCG_U64_EMULATED
    lua_pushboolean(L, 1);
#else
    lua_pushboolean(L, 0);
#endif
    lua_settable(L, -3);

    lua_pushstring(L, "has32bitinteger");
    lua_pushboolean(L, lua_pcg_lua_Integer_has_32bit());
    lua_settable(L, -3);

    lua_pushstring(L, "has64bitinteger");
    lua_pushboolean(L, lua_pcg_lua_Integer_has_64bit());
    lua_settable(L, -3);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    lua_pushstring(L, "__newindex");
    lua_pushcfunction(L, lua_pcg_newindex);
    lua_settable(L, -3);

    lua_pushstring(L, "__metatable");
    lua_pushboolean(L, 0);
    lua_settable(L, -3);

    lua_setmetatable(L, -2);

    return 1;
}
