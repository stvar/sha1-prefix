// Copyright (C) 2019  Stefan Vargyas
// 
// This file is part of Sha1-Prefix.
// 
// Sha1-Prefix is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Sha1-Prefix is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Sha1-Prefix.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __SHA1_PREFIX_COND_H
#define __SHA1_PREFIX_COND_H

#include "va-args.h"

#ifdef SHA1_PREFIX_COND_EXPANDED_NAME
#define SHA1_PREFIX_COND_EXPANDED
#endif

#define ZERO1  ""
#define ZERO2  "\0"
#define ZERO3  "\0\0"
#define ZERO4  "\0\0\0"
#define ZERO5  "\0\0\0\0"
#define ZERO(n) ZERO ## n

#define ISZ0 1
#define ISZ1 0
#define ISZ2 0
#define ISZ3 0
#define ISZ4 0
#define ISZ5 0
#define ISZ6 0
#define ISZ7 0
#define ISZ8 0
#define ISZ9 0
#define ISZ(n) ISZ ## n

#define LE10 1
#define LE11 1
#define LE12 0
#define LE13 0
#define LE14 0
#define LE15 0
#define LE16 0
#define LE17 0
#define LE18 0
#define LE19 0
#define LE1(n) LE1 ## n

#define GT20 0
#define GT21 0
#define GT22 0
#define GT23 1
#define GT24 1
#define GT25 1
#define GT26 1
#define GT27 1
#define GT28 1
#define GT29 1
#define GT210 1
#define GT2(n) GT2 ## n

#define DIV1 0
#define DIV2 1
#define DIV3 1
#define DIV4 2
#define DIV5 2
#define DIV6 3
#define DIV7 3
#define DIV8 4
#define DIV9 4
#define DIV10 5
#define DIV(n) DIV ## n

#define MOD1 1
#define MOD2 0
#define MOD3 1
#define MOD4 0
#define MOD5 1
#define MOD6 0
#define MOD7 1
#define MOD8 0
#define MOD9 1
#define MOD10 0
#define MOD(n) MOD ## n

#define DEC0 0
#define DEC1 0
#define DEC2 1
#define DEC3 2
#define DEC4 3
#define DEC5 4
#define DEC6 5
#define DEC7 6
#define DEC8 7
#define DEC9 8
#define DEC10 9
#define DEC(n) DEC ## n

#ifdef SHA1_PREFIX_COND_EXPANDED
#define PREFIX_COND_(s, n)       \
    VA_ARGS_SELECT_BOOL(ISZ(n),  \
        (*++ s >> 4), (*s >> 4))
#else
#define PREFIX_COND_(s, n) \
        (s[n] >> 4)
#define PREFIX_COND0__(s, n) \
        s[n]
#endif // SHA1_PREFIX_COND_EXPANDED

#ifndef SHA1_PREFIX_COND_PACKED

#define PREFIX_COND0(s, n)         \
    VA_ARGS_SELECT_BOOL(           \
        LE1(n),                    \
        PREFIX_COND0_(s, n),       \
        PREFIX_COND0__(s, DEC(n)))
#define PREFIX_COND1_(s, n)   \
        PREFIX_COND0(s, n) || \
        PREFIX_COND_(s, n)
#define PREFIX_COND1(s, n)   \
    VA_ARGS_SELECT_BOOL(     \
        ISZ(n),              \
        PREFIX_COND1_(s, n), \
        PREFIX_COND_(s, n))

#endif // SHA1_PREFIX_COND_PACKED

#ifdef SHA1_PREFIX_COND_EXPANDED_NAME

#define PREFIX_COND_NAME(s, n)          \
    VA_ARGS_SELECT_BOOL(GT2(n), s,      \
        SHA1_PREFIX_COND_EXPANDED_NAME)
#define PREFIX_COND(s, n)                   \
    ({                                      \
        VA_ARGS_SELECT_BOOL(GT2(n), ,       \
            const uchar_t*                  \
            SHA1_PREFIX_COND_EXPANDED_NAME  \
            = s;)                           \
        VA_ARGS_SELECT_BOOL(                \
            MOD(n),                         \
            PREFIX_COND0(                   \
                PREFIX_COND_NAME(s, n),     \
                DIV(n)),                    \
            PREFIX_COND1(                   \
                PREFIX_COND_NAME(s, n),     \
            DIV(n)));                       \
    })

#elif !defined(SHA1_PREFIX_COND_PACKED)
#define PREFIX_COND(s, n)        \
    VA_ARGS_SELECT_BOOL(         \
        MOD(n),                  \
        PREFIX_COND0(s, DIV(n)), \
        PREFIX_COND1(s, DIV(n)))
#endif

#ifdef SHA1_PREFIX_COND_MEMCMP
#define PREFIX_COND0_(s, n) \
        memcmp(s, ZERO(n), n)
#endif // SHA1_PREFIX_COND_MEMCMP

#if defined(SHA1_PREFIX_COND_INDEXED)

#define PREFIX_COND0__(s, n) \
    s[n]

#elif defined(SHA1_PREFIX_COND_EXPANDED)

#define PREFIX_COND0__(s, n) \
    VA_ARGS_SELECT_BOOL(     \
        ISZ(n), *++ s, *s)

#endif

#if defined(SHA1_PREFIX_COND_INDEXED) || \
    defined(SHA1_PREFIX_COND_EXPANDED)

#define ARG1 0
#define ARG2 0, 1
#define ARG3 0, 1, 2
#define ARG4 0, 1, 2, 3
#define ARG5 0, 1, 2, 3, 4
#define ARG(n) ARG ## n

#define PREFIX_COND0_(s, n) \
    VA_ARGS_REPEAT_ARG(     \
        ||, s,              \
        PREFIX_COND0__,     \
        ARG(n))

#endif // SHA1_PREFIX_COND_INDEXED ||
       // SHA1_PREFIX_COND_EXPANDED

#ifdef SHA1_PREFIX_COND_PACKED

#define PREFIX_COND1(s)  (*(uint8_t*)  s) & 0xf0u
#define PREFIX_COND2(s)   *(uint8_t*)  s
#define PREFIX_COND3(s)  (*(uint16_t*) s) & 0xf0ffu
#define PREFIX_COND4(s)   *(uint16_t*) s
#define PREFIX_COND5(s)  (*(uint32_t*) s) & 0x00f0ffffu
#define PREFIX_COND6(s)  (*(uint32_t*) s) & 0x00ffffffu
#define PREFIX_COND7(s)  (*(uint32_t*) s) & 0xf0ffffffu
#define PREFIX_COND8(s)   *(uint32_t*) s
#define PREFIX_COND9(s)  (*(uint64_t*) s) & 0x000000f0ffffffffu
#define PREFIX_COND10(s) (*(uint64_t*) s) & 0x000000ffffffffffu
#define PREFIX_COND11(s) (*(uint64_t*) s) & 0x0000f0ffffffffffu
#define PREFIX_COND12(s) (*(uint64_t*) s) & 0x0000ffffffffffffu
#define PREFIX_COND13(s) (*(uint64_t*) s) & 0x00f0ffffffffffffu
#define PREFIX_COND14(s) (*(uint64_t*) s) & 0x00ffffffffffffffu
#define PREFIX_COND15(s) (*(uint64_t*) s) & 0xf0ffffffffffffffu
#define PREFIX_COND16(s)  *(uint64_t*) s

#define PREFIX_COND(s, n) PREFIX_COND ## n(s)

#endif // SHA1_PREFIX_COND_PACKED

#if defined(DEBUG_SHA1_PREFIX_COND_EXPR)
#define DEBUG_PREFIX_COND(n) \
    n PREFIX_COND(sum, n)
#elif defined(DEBUG_SHA1_PREFIX_COND_CODE)
#define DEBUG_PREFIX_COND(n)             \
bool prefix_cond_ ## n(const uchar_t* p) \
{ return PREFIX_COND(p, n); }
#endif

#if defined(DEBUG_SHA1_PREFIX_COND_EXPR) || \
    defined(DEBUG_SHA1_PREFIX_COND_CODE)
DEBUG_PREFIX_COND(1)
DEBUG_PREFIX_COND(2)
DEBUG_PREFIX_COND(3)
DEBUG_PREFIX_COND(4)
DEBUG_PREFIX_COND(5)
DEBUG_PREFIX_COND(6)
DEBUG_PREFIX_COND(7)
DEBUG_PREFIX_COND(8)
DEBUG_PREFIX_COND(9)
DEBUG_PREFIX_COND(10)
#ifdef SHA1_PREFIX_COND_PACKED
DEBUG_PREFIX_COND(11)
DEBUG_PREFIX_COND(12)
DEBUG_PREFIX_COND(13)
DEBUG_PREFIX_COND(14)
DEBUG_PREFIX_COND(15)
DEBUG_PREFIX_COND(16)
#endif // SHA1_PREFIX_COND_PACKED
#endif // defined(DEBUG_SHA1_PREFIX_COND_EXPR) ||
       // defined(DEBUG_SHA1_PREFIX_COND_CODE)

#endif // __SHA1_PREFIX_COND_H


