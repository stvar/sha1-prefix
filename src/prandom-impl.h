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

#ifndef PRANDOM_NAME
#error  PRANDOM_NAME is not defined
#endif

#ifndef PRANDOM_RAND
#error  PRANDOM_RAND is not defined
#endif

#define PRANDOM_MAKE_NAME_(n, s) n ## _ ## s
#define PRANDOM_MAKE_NAME(n, s)  PRANDOM_MAKE_NAME_(n, s)

#define PRANDOM_TYPE  PRANDOM_MAKE_NAME(PRANDOM_NAME, t)
#define PRANDOM_INIT  PRANDOM_MAKE_NAME(PRANDOM_NAME, init)
#define PRANDOM_DONE  PRANDOM_MAKE_NAME(PRANDOM_NAME, done)
#define PRANDOM_DATA  PRANDOM_MAKE_NAME(PRANDOM_NAME, data)
#define PRANDOM_ERROR PRANDOM_MAKE_NAME(PRANDOM_NAME, error)
#define PRANDOM_GEN   PRANDOM_MAKE_NAME(PRANDOM_NAME, gen)
#define PRANDOM_PERM  PRANDOM_MAKE_NAME(PRANDOM_NAME, perm)

#define PRANDOM_RAND_TYPE  PRANDOM_MAKE_NAME(PRANDOM_RAND, t)
#define PRANDOM_RAND_INIT  PRANDOM_MAKE_NAME(PRANDOM_RAND, init)
#define PRANDOM_RAND_DONE  PRANDOM_MAKE_NAME(PRANDOM_RAND, done)
#define PRANDOM_RAND_DATA  PRANDOM_MAKE_NAME(PRANDOM_RAND, data)
#define PRANDOM_RAND_ERROR PRANDOM_MAKE_NAME(PRANDOM_RAND, error)
#define PRANDOM_RAND_GEN   PRANDOM_MAKE_NAME(PRANDOM_RAND, gen)

#define PRANDOM_PERM_TYPE      PRANDOM_MAKE_NAME(PRANDOM_PERM, t)
#define PRANDOM_PERM_INIT      PRANDOM_MAKE_NAME(PRANDOM_PERM, init)
#define PRANDOM_PERM_DONE      PRANDOM_MAKE_NAME(PRANDOM_PERM, done)
#define PRANDOM_PERM_GEN_FIRST PRANDOM_MAKE_NAME(PRANDOM_PERM, gen_first)
#define PRANDOM_PERM_GEN_NEXT  PRANDOM_MAKE_NAME(PRANDOM_PERM, gen_next)
#define PRANDOM_PERM_GEN       PRANDOM_MAKE_NAME(PRANDOM_PERM, gen)

// stev: Knuth, TAOCP, vol. 4A
// Combinatorial algorithms, part 1
// 7.2.1.2 Generating all permutations
// Algorithm P, p. 322

struct PRANDOM_PERM_TYPE
{
    struct random_data_t
            data;
    int    *inversions,
           *directions;
    bits_t  done: 1;
};

static inline void
    PRANDOM_PERM_INIT(
        struct PRANDOM_PERM_TYPE* perm,
        struct random_data_t data)
{
    ASSERT(data.size > 1);
    perm->data = data;

    perm->inversions = malloc(SIZE_MUL(
        data.size, SZ(2) * sizeof(int)));
    VERIFY(perm->inversions != NULL);

    perm->directions =
        perm->inversions + data.size;
    perm->done = true;
}

static inline void 
    PRANDOM_PERM_DONE(
        struct PRANDOM_PERM_TYPE* perm)
{
    free(perm->inversions);
}

static inline bool
    PRANDOM_PERM_GEN_FIRST(
        struct PRANDOM_PERM_TYPE* perm)
{
    size_t n = perm->data.size, i;

    // step P1: Initialize
    for (i = 0; i < n; i ++) {
        perm->inversions[i] = 0;
        perm->directions[i] = 1;
    }

    perm->done = false;
    return true;
}

#define a(k) A[(k) - 1]
#define c(k) C[(k) - 1]
#define o(k) O[(k) - 1]

static inline bool
    PRANDOM_PERM_GEN_NEXT(
        struct PRANDOM_PERM_TYPE* perm)
{
    int *C, *O, j, s, q;
    uchar_t *A, *x, *y;

    ASSERT(!perm->done);

    A = perm->data.ptr;
    C = perm->inversions;
    O = perm->directions;

    // step P2: Visit
    // stev: already done that

    // step P3: Prepare for change
    j = SIZE_AS_INT(perm->data.size);
    s = 0;

    while (true) {
        // step P4: Ready to change?
        q = c(j) + o(j);
        if (q < 0)
            goto P7;
        if (q == j)
            goto P6;

        // step P5: Change
        // Interchange a[j - c[j] + s] and a[j - q + s] 
        x = &a(j - c(j) + s), y = &a(j - q + s); 
        *x ^= *y;
        *y ^= *x;
        *x ^= *y;

        c(j) = q;
        // goto P2; i.e. Visit
        return true;

    P6: // step P6: Increase s
        if (j == 1) {
            perm->done = true;
            return false;
        }
        s ++;

    P7: // step P7: Switch direction
        o(j) = -o(j);
        j --;
        // goto P4
    }
}

#undef o
#undef c
#undef a

static inline bool
    PRANDOM_PERM_GEN(
        struct PRANDOM_PERM_TYPE* perm)
{
    if (UNLIKELY(perm->done))
        return PRANDOM_PERM_GEN_FIRST(perm);
    else
        return PRANDOM_PERM_GEN_NEXT(perm);
}

struct PRANDOM_TYPE
{
    struct PRANDOM_RAND_TYPE rand;
    struct PRANDOM_PERM_TYPE perm;
};

static bool PRANDOM_INIT(
    struct PRANDOM_TYPE* prand,
    size_t size)
{
    memset(prand, 0, sizeof(*prand));

    if (!PRANDOM_RAND_INIT(&prand->rand, size))
        return false;

    PRANDOM_RAND_GEN(&prand->rand);
    PRANDOM_PERM_INIT(&prand->perm,
        PRANDOM_RAND_DATA(&prand->rand));

    return true;
}

static void PRANDOM_DONE(
    struct PRANDOM_TYPE* prand)
{
    PRANDOM_PERM_DONE(&prand->perm);
    PRANDOM_RAND_DONE(&prand->rand);
}

static struct random_data_t
    PRANDOM_DATA(struct PRANDOM_TYPE* prand)
{
    return prand->perm.data;
}

static void PRANDOM_ERROR(
    const struct PRANDOM_TYPE* prand)
{
    PRANDOM_RAND_ERROR(&prand->rand);
}

static inline void
    PRANDOM_GEN(struct PRANDOM_TYPE* prand)
{
    if (UNLIKELY(!PRANDOM_PERM_GEN(&prand->perm))) {
        PRANDOM_RAND_GEN(&prand->rand);
        PRANDOM_PERM_GEN(&prand->perm);
        ASSERT(!prand->perm.done);
    }
}


