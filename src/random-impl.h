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

#ifndef RANDOM_NAME
#error  RANDOM_NAME is not defined
#endif

#ifndef RANDOM_BITS
#error  RANDOM_BITS is not defined
#elif   RANDOM_BITS != 32 && RANDOM_BITS != 64
#error  RANDOM_BITS is not 32 neither 64
#endif

#ifdef  RANDOM_NEED_GEN_ONLY
#ifndef RANDOM_BASE
#error  RANDOM_BASE is not defined
#endif
#endif // RANDOM_NEED_GEN_ONLY

#ifndef RANDOM_NEED_GEN_ONLY
#ifndef RANDOM_GEN_SEED
#error  RANDOM_GEN_SEED is not defined
#endif
#endif // RANDOM_NEED_GEN_ONLY

#ifndef RANDOM_GEN_RAND
#error  RANDOM_GEN_RAND is not defined
#endif

#define RANDOM_MAKE_NAME_(n, s) n ## _ ## s
#define RANDOM_MAKE_NAME(n, s)  RANDOM_MAKE_NAME_(n, s)

#define RANDOM_TYPE  RANDOM_MAKE_NAME(RANDOM_NAME, t)
#define RANDOM_INIT  RANDOM_MAKE_NAME(RANDOM_NAME, init)
#define RANDOM_DONE  RANDOM_MAKE_NAME(RANDOM_NAME, done)
#define RANDOM_DATA  RANDOM_MAKE_NAME(RANDOM_NAME, data)
#define RANDOM_ERROR RANDOM_MAKE_NAME(RANDOM_NAME, error)
#define RANDOM_GEN   RANDOM_MAKE_NAME(RANDOM_NAME, gen)

#define RANDOM_SEED_SIZE RANDOM_MAKE_NAME(RANDOM_NAME, seed_size)
#define RANDOM_RAND_SIZE RANDOM_MAKE_NAME(RANDOM_NAME, rand_size)

#define RANDOM_SEED_TYPE RANDOM_MAKE_NAME(RANDOM_NAME, seed_t)

#undef  RANDOM_RAND_TYPE
#ifndef RANDOM_NEED_GEN_ONLY
#define RANDOM_RAND_TYPE RANDOM_MAKE_NAME(RANDOM_NAME, rand_t)
#else // RANDOM_NEED_GEN_ONLY
#define RANDOM_RAND_TYPE RANDOM_MAKE_NAME(RANDOM_BASE, rand_t)
#endif // RANDOM_NEED_GEN_ONLY

#define RANDOM_UINT_TYPE_(n) uint ## n ## _t
#define RANDOM_UINT_TYPE(n)  RANDOM_UINT_TYPE_(n)

#undef  RANDOM_GEN_RAND_TYPE
#define RANDOM_GEN_RAND_TYPE RANDOM_UINT_TYPE(RANDOM_BITS)

#ifndef RANDOM_NEED_GEN_ONLY

struct RANDOM_TYPE
{
    RANDOM_GEN_TYPE gen;
    struct urandom_error_t err;
    uchar_t* buf;
    size_t size;
    size_t len;
};

enum {
    RANDOM_SEED_SIZE =
        sizeof(RANDOM_GEN_SEED_TYPE),
    RANDOM_RAND_SIZE =
        sizeof(RANDOM_GEN_RAND_TYPE)
};

union RANDOM_RAND_TYPE
{
    uchar_t raw[RANDOM_RAND_SIZE];
    RANDOM_GEN_RAND_TYPE val;
};

union RANDOM_SEED_TYPE
{
    uchar_t raw[RANDOM_SEED_SIZE];
    RANDOM_GEN_SEED_TYPE val;
};

static bool RANDOM_INIT(
    struct RANDOM_TYPE* rand,
    size_t size)
{
    struct urandom_t u;

    memset(rand, 0, sizeof(*rand));

    VERIFY(size > 1);
    rand->size = size;
    rand->len = size / RANDOM_RAND_SIZE;

    if (size % RANDOM_RAND_SIZE) {
        size += RANDOM_RAND_SIZE;
        rand->len ++;
    }

    rand->buf = malloc(size);
    VERIFY(rand->buf != NULL);

    ASSERT(PTR_IS_ALIGNED(
        rand->buf, union RANDOM_RAND_TYPE));

    if (!urandom_init(&u, RANDOM_SEED_SIZE) ||
        !urandom_read(&u)) {
        rand->err = u.err;
        urandom_done(&u);
        return false;
    }

    ASSERT(PTR_IS_ALIGNED(
        u.buf, union RANDOM_SEED_TYPE));
    RANDOM_GEN_SEED(&rand->gen,
        ((union RANDOM_SEED_TYPE*) u.buf)->val);

    urandom_done(&u);
    return true;
}

static void RANDOM_DONE(
    struct RANDOM_TYPE* rand)
{
    free(rand->buf);
}

static struct random_data_t
    RANDOM_DATA(struct RANDOM_TYPE* rand)
{
    return (struct random_data_t) {
        .size = rand->size,
        .ptr = rand->buf
    };
}

static void RANDOM_ERROR(
    const struct RANDOM_TYPE* rand)
{
    urandom_error_print(&rand->err);
}

#endif // RANDOM_NEED_GEN_ONLY

static inline void RANDOM_GEN(
    struct RANDOM_TYPE* rand)
{
    union RANDOM_RAND_TYPE *e, *p =
        (union RANDOM_RAND_TYPE*) rand->buf;

    // stev: rand->len > 0 (see RANDOM_INIT);
    // ASSERTing it implies speed penalties

    switch (rand->len) {
    default:
        e = p + rand->len - 8;
        while (p < e)
            (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 8: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 7: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 6: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 5: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 4: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 3: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 2: (p ++)->val = RANDOM_GEN_RAND(&rand->gen);
            FALLTHROUGH;
    case 1:  p->val     = RANDOM_GEN_RAND(&rand->gen);
    }
}


