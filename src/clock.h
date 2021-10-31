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

#ifndef __CLOCK_H
#define __CLOCK_H

#define CLOCK_SUB(x, y)                 \
    ({                                  \
        STATIC(TYPEOF_IS(x, uint64_t)); \
        STATIC(TYPEOF_IS(y, uint64_t)); \
        ASSERT((x) >= (y));             \
        (x) - (y);                      \
    })

// Gabriele Paoloni:
// How to Benchmark Code Execution Times on Intel
// IA-32 and IA-64 Instruction Set Architectures.
// White Paper, Intel Corporation, September 2010
// https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/
// ia-32-ia-64-benchmark-code-execution-paper.pdf

#define CLOCK_GET_RDTSC_CYCLES()    \
    ({                              \
        uint32_t __l, __h;          \
        asm volatile (              \
            "cpuid\n"               \
            "rdtsc\n"               \
            "mov %%edx, %0\n"       \
            "mov %%eax, %1\n" :     \
            "=r" (__h),             \
            "=r" (__l) ::           \
            "%rax", "%rbx",         \
            "%rcx", "%rdx"          \
        );                          \
        (uint64_t) __h << 32 | __l; \
    })
#define CLOCK_GET_RDTSCP_CYCLES()   \
    ({                              \
        uint32_t __l, __h;          \
        asm volatile (              \
            "rdtscp\n"              \
            "mov %%edx, %0\n"       \
            "mov %%eax, %1\n"       \
            "cpuid\n" :             \
            "=r" (__h),             \
            "=r" (__l) ::           \
            "%rax", "%rbx",         \
            "%rcx", "%rdx"          \
        );                          \
        (uint64_t) __h << 32 | __l; \
    })

#define CLOCK_START CLOCK_GET_RDTSC_CYCLES
#define CLOCK_STOP  CLOCK_GET_RDTSCP_CYCLES

#endif // __CLOCK_H


