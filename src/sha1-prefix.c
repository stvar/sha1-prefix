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

#include "config.h"

#define _GNU_SOURCE
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#ifdef SHA1_PREFIX_OPENSSL_SHA1
#include <openssl/sha.h>
#else // SHA1_PREFIX_OPENSSL_SHA1
#include "sha1.h"
#define SHA_DIGEST_LENGTH 20
#endif // SHA1_PREFIX_OPENSSL_SHA1

#include <pcg_variants.h>
#include <SFMT.h>

#ifdef static_assert
#undef static_assert
#endif

#include "char-traits.h"
#include "ptr-traits.h"
#include "file-buf.h"
#include "su-size.h"
#include "common.h"
#include "clock.h"

#define   MT19937_UINT_TYPE uint32_t
#include "mt19937-impl.h"

#define   SHA1_PREFIX_COND_PACKED
#include "sha1-prefix-cond.h"

#define ALIGNED(n)  __attribute__ ((aligned(n)))
#define FALLTHROUGH __attribute__ ((fallthrough))

#define PTR_IS_ALIGNED(p, t)           \
    ({                                 \
        enum { __a = MEM_ALIGNOF(t) }; \
        size_t __p = PTR_TO_SIZE(p);   \
        __p % __a == 0;                \
    })

#define ISASCII CHAR_IS_ASCII

const char stdin_name[] = "<stdin>";

const char program[] = STRINGIFY(PROGRAM);
const char verdate[] = "0.1 -- 2019-06-28 18:29"; // $ date +'%F %R'

const char help[] =
"usage: %s [ACTION|OPTION]... [FILE]\n"
"where actions are:\n"
#ifdef SHA1_PREFIX_COND_PACKED
"  -C|--prefix-cond        generate prefix condition test\n"
#endif // SHA1_PREFIX_COND_PACKED
#ifndef SHA1_PREFIX_OPENSSL_SHA1
"  -D|--sha1-digest        compute SHA1 digest sum\n"
#endif // SHA1_PREFIX_OPENSSL_SHA1
"  -S|--gen-strings        generate random strings\n"
"  -P|--sha1-prefix        generate SHA1 prefix (default)\n"
"and options are:\n"
"  -e|--[no-]sighup-exits  exit the prefix loop (and the program\n"
"                            for that matter) when signaled with\n"
"                            SIGHUP (default do not, meaning the\n"
"                            program gets killed)\n"
"  -g|--random-gen=NAME    when generating random strings --\n"
"                            the action is `-S|--gen-strings'\n"
"                            or `-P|--sha1-prefix' -- use the\n"
"                            specified algorithm: '[pa]d48',\n"
"                            '[pa]mt', '[pa]pcg' or '[pa]sfmt'\n"
"                            (the default is 'sfmt')\n"
"  -m|--input-max=NUM[KB]  the maximum size of the input file\n"
"                            buffer (default 40K; not allowed\n"
"                            to be more than 1M)\n"
"  -n|--strings=NUM        when generating random strings,\n"
"                            do stop the algorithm when the\n"
"                            the number of iterations reaches\n"
"                            the specified limit (the default\n"
"                            is '-', i.e. SIZE_MAX)\n"
"  -s|--length=NUM         when generating random strings, do\n"
"                            produce them of specified length\n"
"                            (the default: 16)\n"
"  -w|--width=NUM          when action is `-P|--sha1-prefix',\n"
"                            try to produce SHA1 prefixes of\n"
"                            specified width (default: 6)\n";

typedef char fmt_buf_t[512];

void error(
    const char* fmt, ...)
{
    va_list arg;

    fmt_buf_t b;

    va_start(arg, fmt);
    vsnprintf(b, sizeof b - 1, fmt, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    fprintf(
        stderr, "%s: error: %s\n",
        program, b);

    exit(127);
}

void assert_failed(
    const char* file, int line,
    const char* func, const char* expr)
{
    error("assertion failed: %s:%d:%s: %s",
        file, line, func, expr);
}

void ensure_failed(
    const char* file, int line,
    const char* func, const char* msg, ...)
{
    va_list arg;
    fmt_buf_t b;

    va_start(arg, msg);
    vsnprintf(b, sizeof b - 1, msg, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    error("%s:%d:%s: %s",
        file, line, func, b);
}

void unexpect_error(
    const char* file, int line,
    const char* func, const char* msg, ...)
{
    va_list arg;
    fmt_buf_t b;

    va_start(arg, msg);
    vsnprintf(b, sizeof b - 1, msg, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    error("unexpected error: %s:%d:%s: %s",
        file, line, func, b);
}

enum options_action_t
{
    options_action_prefix_cond,
#ifndef SHA1_PREFIX_OPENSSL_SHA1
    options_action_sha1_digest,
#endif // SHA1_PREFIX_OPENSSL_SHA1
    options_action_gen_strings,
    options_action_sha1_prefix,
};

enum options_random_gen_t
{
    // stev: basic generators
    options_random_gen_d48,
    options_random_gen_mt,
    options_random_gen_pcg,
    options_random_gen_sfmt,
    // stev: composed generators
    options_random_gen_pd48,
    options_random_gen_pmt,
    options_random_gen_ppcg,
    options_random_gen_psfmt,
    options_random_gen_ad48,
    options_random_gen_amt,
    options_random_gen_apcg,
    options_random_gen_asfmt,
};

struct options_t
{
    enum options_action_t
                 action;
    enum options_random_gen_t
                 random_gen;
    size_t       input_max;
    size_t       strings;
    size_t       length;
    size_t       width;
    const char*  input;
    bits_t       sighup: 1;
};

static void options_invalid_opt_arg(
    const char* opt_name, const char* opt_arg)
{
    error("invalid argument for '%s' option: '%s'",
        opt_name, opt_arg);
}

static void options_illegal_opt_arg(
    const char* opt_name, const char* opt_arg)
{
    error("illegal argument for '%s' option: '%s'",
        opt_name, opt_arg);
}

#define SU_SIZE_OPT_NAME options
#define SU_SIZE_OPT_NEED_PARSE_SIZE
#define SU_SIZE_OPT_NEED_PARSE_SIZE_SU
#include "su-size-opt-impl.h"

// $ . ~/trie-gen/commands.sh
// $ print() { printf '%s\n' "$@"; }
// $ adjust-func() { ssed -R '1s/^/static /;1s/\&/*/;1s/(,\s+)/\1enum /;s/(?=t =)/*/;1s/(?<=\()/\n\t/;s/_t::/_/'; }

// $ print {,p,a}{d48,mt,pcg,sfmt}|gen-func -f options_lookup_random_gen -r options_random_gen_t -Pe|adjust-func

static bool options_lookup_random_gen(
    const char* n, enum options_random_gen_t* t)
{
    // pattern: a(d48|mt|pcg|sfmt)|d48|mt|p(cg|d48|mt|pcg|sfmt)|sfmt
    switch (*n ++) {
    case 'a':
        switch (*n ++) {
        case 'd':
            if (*n ++ == '4' &&
                *n ++ == '8' &&
                *n == 0) {
                *t = options_random_gen_ad48;
                return true;
            }
            return false;
        case 'm':
            if (*n ++ == 't' &&
                *n == 0) {
                *t = options_random_gen_amt;
                return true;
            }
            return false;
        case 'p':
            if (*n ++ == 'c' &&
                *n ++ == 'g' &&
                *n == 0) {
                *t = options_random_gen_apcg;
                return true;
            }
            return false;
        case 's':
            if (*n ++ == 'f' &&
                *n ++ == 'm' &&
                *n ++ == 't' &&
                *n == 0) {
                *t = options_random_gen_asfmt;
                return true;
            }
        }
        return false;
    case 'd':
        if (*n ++ == '4' &&
            *n ++ == '8' &&
            *n == 0) {
            *t = options_random_gen_d48;
            return true;
        }
        return false;
    case 'm':
        if (*n ++ == 't' &&
            *n == 0) {
            *t = options_random_gen_mt;
            return true;
        }
        return false;
    case 'p':
        switch (*n ++) {
        case 'c':
            if (*n ++ == 'g' &&
                *n == 0) {
                *t = options_random_gen_pcg;
                return true;
            }
            return false;
        case 'd':
            if (*n ++ == '4' &&
                *n ++ == '8' &&
                *n == 0) {
                *t = options_random_gen_pd48;
                return true;
            }
            return false;
        case 'm':
            if (*n ++ == 't' &&
                *n == 0) {
                *t = options_random_gen_pmt;
                return true;
            }
            return false;
        case 'p':
            if (*n ++ == 'c' &&
                *n ++ == 'g' &&
                *n == 0) {
                *t = options_random_gen_ppcg;
                return true;
            }
            return false;
        case 's':
            if (*n ++ == 'f' &&
                *n ++ == 'm' &&
                *n ++ == 't' &&
                *n == 0) {
                *t = options_random_gen_psfmt;
                return true;
            }
        }
        return false;
    case 's':
        if (*n ++ == 'f' &&
            *n ++ == 'm' &&
            *n ++ == 't' &&
            *n == 0) {
            *t = options_random_gen_sfmt;
            return true;
        }
    }
    return false;
}

static enum options_random_gen_t
    options_parse_random_gen_optarg(
        const char* opt_name, const char* opt_arg)
{
    enum options_random_gen_t r;

    ASSERT(opt_arg != NULL);
    if (!options_lookup_random_gen(opt_arg, &r))
        options_invalid_opt_arg(opt_name, opt_arg);

    return r;
}

static inline size_t
    options_parse_strings_optarg(
        const char* opt_name, const char* opt_arg)
{
    return strcmp(opt_arg, "-")
        ? options_parse_size_optarg(
                opt_name, opt_arg, 1, 0)
        : SIZE_MAX;
}

static const struct options_t* options(
    int argc, char* argv[])
{
    static struct options_t opts = {
        .action     = options_action_sha1_prefix,
        .random_gen = options_random_gen_mt,
        .input_max  = KB(40),
        .strings    = SIZE_MAX,
        .length     = 16,
        .width      = 6,
        .input      = NULL,
        .sighup     = false,
    };

    enum {
        // stev: actions:
        prefix_cond_act     = 'C',
#ifndef SHA1_PREFIX_OPENSSL_SHA1
        sha1_digest_act     = 'D',
#endif // SHA1_PREFIX_OPENSSL_SHA1
        gen_strings_act     = 'S',
        sha1_prefix_act     = 'P',

        // stev: options:
        sighup_exits_opt    = 'e',
        random_gen_opt      = 'g',
        input_max_opt       = 'm',
        strings_opt         = 'n',
        length_opt          = 's',
        width_opt           = 'w',

        help_opt            = '?',

        no_sighup_exits_opt = 128,
        version_opt,
    };

    static const struct option longs[] = {
        { "prefix-cond",     0,       0, prefix_cond_act },
#ifndef SHA1_PREFIX_OPENSSL_SHA1
        { "sha1-digest",     0,       0, sha1_digest_act },
#endif // SHA1_PREFIX_OPENSSL_SHA1
        { "gen-strings",     0,       0, gen_strings_act },
        { "sha1-prefix",     0,       0, sha1_prefix_act },
        { "sighup-exits",    0,       0, sighup_exits_opt },
        { "no-sighup-exits", 0,       0, no_sighup_exits_opt },
        { "random-gen",      1,       0, random_gen_opt },
        { "input-max",       1,       0, input_max_opt },
        { "strings",         1,       0, strings_opt },
        { "length",          1,       0, length_opt },
        { "width",           1,       0, width_opt },
        { "version",         0,       0, version_opt },
        { "help",            0, &optopt, help_opt },
        { 0,                 0,       0, 0 }
    };
    static const char shorts[] = ":"
        "CPS"
#ifndef SHA1_PREFIX_OPENSSL_SHA1
        "D"
#endif // SHA1_PREFIX_OPENSSL_SHA1
        "eg:m:n:s:w:";

    struct bits_opts_t
    {
        bits_t usage: 1;
        bits_t version: 1;
    };
    struct bits_opts_t bits = {
        .usage   = false,
        .version = false
    };
    int opt;

#define argv_optind()                      \
    ({                                     \
        size_t i = INT_AS_SIZE(optind);    \
        ASSERT_SIZE_DEC_NO_OVERFLOW(i);    \
        ASSERT(i - 1 < INT_AS_SIZE(argc)); \
        argv[i - 1];                       \
    })

#define optopt_char()                   \
    ({                                  \
        ASSERT(ISASCII((char) optopt)); \
        (char) optopt;                  \
    })

#define missing_opt_arg_str(n) \
    error("argument for option '%s' not found", n)
#define missing_opt_arg_ch(n) \
    error("argument for option '-%c' not found", n);
#define not_allowed_opt_arg(n) \
    error("option '%s' does not allow an argument", n)
#define invalid_opt_str(n) \
    error("invalid command line option '%s'", n)
#define invalid_opt_ch(n) \
    error("invalid command line option '-%c'", n)

    opterr = 0;
    optind = 1;
    while ((opt = getopt_long(
        argc, argv, shorts, longs, 0)) != EOF) {
        switch (opt) {
        case prefix_cond_act:
            opts.action = options_action_prefix_cond;
            break;
#ifndef SHA1_PREFIX_OPENSSL_SHA1
        case sha1_digest_act:
            opts.action = options_action_sha1_digest;
            break;
#endif // SHA1_PREFIX_OPENSSL_SHA1
        case gen_strings_act:
            opts.action = options_action_gen_strings;
            break;
        case sha1_prefix_act:
            opts.action = options_action_sha1_prefix;
            break;
        case sighup_exits_opt:
            opts.sighup = true;
            break;
        case no_sighup_exits_opt:
            opts.sighup = false;
            break;
        case random_gen_opt:
            opts.random_gen = options_parse_random_gen_optarg(
                "random-gen", optarg);
            break;
        case input_max_opt:
            opts.input_max = options_parse_su_size_optarg(
                "input-max", optarg, 1, MB(1));
            break;
        case strings_opt:
            opts.strings = options_parse_strings_optarg(
                "strings", optarg);
            break;
        case length_opt:
            opts.length = options_parse_size_optarg(
                "length", optarg, 1, 64);
            break;
        case width_opt:
            opts.width = options_parse_size_optarg(
                "width", optarg, 1, 10);
            break;
        case version_opt:
            bits.version = true;
            break;
        case 0:
            bits.usage = true;
            break;
        case ':': {
            const char* opt = argv_optind();
            if (opt[0] == '-' && opt[1] == '-')
                missing_opt_arg_str(opt);
            else
                missing_opt_arg_ch(optopt_char());
            break;
        }
        case '?':
        default:
            if (optopt == 0)
                invalid_opt_str(argv_optind());
            else
            if (optopt != '?') {
                char* opt = argv_optind();
                if (opt[0] == '-' && opt[1] == '-') {
                    char* end = strchr(opt, '=');
                    if (end) *end = '\0';
                    not_allowed_opt_arg(opt);
                }
                else
                    invalid_opt_ch(optopt_char());
            }
            else
                bits.usage = true;
            break;
        }
    }

    ASSERT(optind > 0);
    ASSERT(optind <= argc);

    argc -= optind;
    argv += optind;

    if (argc > 0) {
        opts.input = *argv ++;
        argc --;
    }

    if (bits.version)
        printf("%s: version %s\n",
            program, verdate);
    if (bits.usage)
        printf(help, program);

    if (bits.version ||
        bits.usage)
        exit(0);

    if (opts.input != NULL &&
        !strcmp(opts.input, "-"))
        opts.input = NULL;

    return &opts;
}

#undef  CASE
#define CASE(t) \
    [options_random_gen_ ## t] = #t

static void options_print_random_gen_name(
    enum options_random_gen_t gen)
{
    static const char* const gens[] = {
        CASE(d48),
        CASE(mt),
        CASE(pcg),
        CASE(sfmt),
        CASE(pd48),
        CASE(pmt),
        CASE(ppcg),
        CASE(psfmt),
        CASE(ad48),
        CASE(amt),
        CASE(apcg),
        CASE(asfmt),
    };
    const char* t = ARRAY_NULL_ELEM(
        gens, gen);

    ASSERT(t != NULL);
    fputs(t, stdout);
}

#define input_t    file_buf_t
#define input_done file_buf_done

static bool input_init(
    struct input_t* input,
    const char* file_name,
    size_t max_size)
{
    file_buf_init(input, file_name, max_size); 

    return input->error_info.type ==
        file_buf_error_none;
}

#undef  CASE
#define CASE(n) [file_buf_error_file_ ## n] = #n

static void input_error(
    const struct input_t* input)
    NORETURN;

static void input_error(
    const struct input_t* input)
{
    static const char* const types[] = {
        CASE(open),
        CASE(stat),
        CASE(read),
        CASE(close),
    };
    const char* t = ARRAY_NULL_ELEM(types,
        input->error_info.type);

    ASSERT(t != NULL);

    error("%s: %s error: %s",
        input->file_name
        ? input->file_name : stdin_name,
        t, strerror(input->error_info.sys_error));
}

struct random_data_t
{
    uchar_t* ptr;
    size_t size;
};

enum urandom_error_type_t
{
    urandom_error_none,
    urandom_error_open,
    urandom_error_read,
};

struct urandom_error_t
{
    enum urandom_error_type_t type;
    int sys;
};

struct urandom_t
{
    struct urandom_error_t err;
    uchar_t *buf;
    size_t size;
    int fid;
};

#define URANDOM_ERROR(t)         \
    ({                           \
        urand->err.type =        \
            urandom_error_ ## t; \
        urand->err.sys = errno;  \
        false;                   \
    })

static bool urandom_init(
    struct urandom_t* urand,
    size_t size)
{
    memset(urand, 0, sizeof(*urand));

    VERIFY(size > 1);
    urand->size = size;

    urand->buf = malloc(size);
    VERIFY(urand->buf != NULL);

    urand->fid = open("/dev/urandom", O_RDONLY);
    if (urand->fid < 0)
        return URANDOM_ERROR(open);

    return true;
}

static void urandom_done(
    struct urandom_t* urand)
{
    close(urand->fid);
    free(urand->buf);
}

static bool urandom_read(
    struct urandom_t* urand)
{
    ssize_t n;

    if (urand->err.type)
        return false;

    n = read(urand->fid, urand->buf, urand->size);
    if (n < 0 || INT_AS_SIZE(n) != urand->size)
        return URANDOM_ERROR(read);

    return true;
}

#undef  CASE
#define CASE(n) [urandom_error_ ## n] = #n

static void urandom_error_print(
    const struct urandom_error_t* err)
{
    static const char* const types[] = {
        CASE(none),
        CASE(open),
        CASE(read),
    };
    const char* t = ARRAY_NULL_ELEM(
        types, err->type);

    ASSERT(t != NULL);

    error("\"/dev/urandom\": %s error: %s",
        t, strerror(err->sys));
}

// struct drandom_t

#define LRAND48(g)                  \
    ({                              \
        long int __r;               \
        STATIC(TYPEOF_IS(g,         \
            struct drand48_data*)); \
        lrand48_r(g, &__r);         \
        __r;                        \
    })

typedef unsigned short ushort_array3_t[3];

#define RANDOM_BITS           32
#define RANDOM_NAME           drandom
#define RANDOM_GEN_TYPE       struct drand48_data
#define RANDOM_GEN_SEED_TYPE  ushort_array3_t
#define RANDOM_GEN_SEED(g, s) seed48_r(s, g)
#define RANDOM_GEN_RAND(g)    LRAND48(g)

#include "random-impl.h"

#undef  RANDOM_GEN_RAND
#undef  RANDOM_GEN_SEED
#undef  RANDOM_GEN_SEED_TYPE
#undef  RANDOM_GEN_TYPE
#undef  RANDOM_NAME
#undef  RANDOM_BITS

// struct mrandom_t

typedef mt19937_uint_t mt19937_init_array_t[MT19937_N];

#define RANDOM_BITS           32
#define RANDOM_NAME           mrandom
#define RANDOM_GEN_TYPE       struct mt19937_t
#define RANDOM_GEN_SEED_TYPE  mt19937_init_array_t
#define RANDOM_GEN_SEED(g, s) mt19937_init_by_array(g, s, MT19937_N)
#define RANDOM_GEN_RAND(g)    mt19937_genrand_int32(g)

#include "random-impl.h"

#undef  RANDOM_GEN_RAND
#undef  RANDOM_GEN_SEED
#undef  RANDOM_GEN_SEED_TYPE
#undef  RANDOM_GEN_TYPE
#undef  RANDOM_NAME
#undef  RANDOM_BITS

// struct nrandom_t

#if SHA1_PREFIX_PCG_BITS != 32 && \
    SHA1_PREFIX_PCG_BITS != 64
#error SHA1_PREFIX_PCG_BITS is neither 32 not 64
#endif

#define RANDOM_BITS           SHA1_PREFIX_PCG_BITS
#define RANDOM_NAME           nrandom

#if SHA1_PREFIX_PCG_BITS == 64

typedef pcg128_t pcg128_array2_t[2];

#define RANDOM_GEN_TYPE       pcg64_random_t
#define RANDOM_GEN_SEED_TYPE  pcg128_array2_t
#define RANDOM_GEN_SEED(g, s) pcg64_srandom_r(g, s[0], s[1])
#define RANDOM_GEN_RAND(g)    pcg64_random_r(g)

#else // SHA1_PREFIX_PCG_BITS == 64

typedef uint64_t uint64_array2_t[2];

#define RANDOM_GEN_TYPE       pcg32_random_t
#define RANDOM_GEN_SEED_TYPE  uint64_array2_t
#define RANDOM_GEN_SEED(g, s) pcg32_srandom_r(g, s[0], s[1])
#define RANDOM_GEN_RAND(g)    pcg32_random_r(g)

#endif // SHA1_PREFIX_PCG_BITS == 64

#include "random-impl.h"

#undef  RANDOM_GEN_RAND
#undef  RANDOM_GEN_SEED
#undef  RANDOM_GEN_SEED_TYPE
#undef  RANDOM_GEN_TYPE
#undef  RANDOM_NAME
#undef  RANDOM_BITS

// struct srandom_t

#if SHA1_PREFIX_SFMT_BITS != 32 && \
    SHA1_PREFIX_SFMT_BITS != 64
#error SHA1_PREFIX_SFMT_BITS is neither 32 not 64
#endif

typedef uint32_t sfmt_init_array_t[SFMT_N32];

#define RANDOM_BITS           SHA1_PREFIX_SFMT_BITS

#define RANDOM_NAME           srandom
#define RANDOM_GEN_TYPE       sfmt_t
#define RANDOM_GEN_SEED_TYPE  sfmt_init_array_t
#define RANDOM_GEN_SEED(g, s) sfmt_init_by_array(g, s, SFMT_N32)
#if SHA1_PREFIX_SFMT_BITS == 64
#define RANDOM_GEN_RAND(g)    sfmt_genrand_uint64(g)
#else // SHA1_PREFIX_SFMT_BITS == 64
#define RANDOM_GEN_RAND(g)    sfmt_genrand_uint32(g)
#endif // SHA1_PREFIX_SFMT_BITS == 64

#include "random-impl.h"

#undef  RANDOM_GEN_RAND
#undef  RANDOM_GEN_SEED
#undef  RANDOM_GEN_SEED_TYPE
#undef  RANDOM_GEN_TYPE
#undef  RANDOM_NAME

#undef  RANDOM_BITS

// struct pdrandom_t

#define   PRANDOM_NAME pdrandom
#define   PRANDOM_RAND drandom
#include "prandom-impl.h"
#undef    PRANDOM_RAND
#undef    PRANDOM_NAME

// struct pmrandom_t

#define   PRANDOM_NAME pmrandom
#define   PRANDOM_RAND mrandom
#include "prandom-impl.h"
#undef    PRANDOM_RAND
#undef    PRANDOM_NAME

// struct pnrandom_t

#define   PRANDOM_NAME pnrandom
#define   PRANDOM_RAND nrandom
#include "prandom-impl.h"
#undef    PRANDOM_RAND
#undef    PRANDOM_NAME

// struct psrandom_t

#define   PRANDOM_NAME psrandom
#define   PRANDOM_RAND srandom
#include "prandom-impl.h"
#undef    PRANDOM_RAND
#undef    PRANDOM_NAME

// struct adrandom_t

#define adrandom_t     drandom_t 
#define adrandom_init  drandom_init
#define adrandom_done  drandom_done
#define adrandom_data  drandom_data
#define adrandom_error drandom_error

#define   ARANDOM_BITS     32
#define   ARANDOM_BASE     drandom
#define   ARANDOM_NAME     adrandom
#define   ARANDOM_GEN_TYPE struct drand48_data
#define   ARANDOM_GEN_RAND LRAND48
#include "arandom-impl.h"
#undef    ARANDOM_GEN_RAND
#undef    ARANDOM_GEN_TYPE
#undef    ARANDOM_NAME
#undef    ARANDOM_BASE
#undef    ARANDOM_BITS

// struct amrandom_t

#define amrandom_t     mrandom_t 
#define amrandom_init  mrandom_init
#define amrandom_done  mrandom_done
#define amrandom_data  mrandom_data
#define amrandom_error mrandom_error

#define   ARANDOM_BITS     32
#define   ARANDOM_BASE     mrandom
#define   ARANDOM_NAME     amrandom
#define   ARANDOM_GEN_TYPE struct mt19937_t
#define   ARANDOM_GEN_RAND mt19937_genrand_int32
#include "arandom-impl.h"
#undef    ARANDOM_GEN_RAND
#undef    ARANDOM_GEN_TYPE
#undef    ARANDOM_NAME
#undef    ARANDOM_BASE
#undef    ARANDOM_BITS

// struct anrandom_t

#define anrandom_t     nrandom_t 
#define anrandom_init  nrandom_init
#define anrandom_done  nrandom_done
#define anrandom_data  nrandom_data
#define anrandom_error nrandom_error

#define   ARANDOM_BASE     nrandom
#define   ARANDOM_NAME     anrandom
#define   ARANDOM_BITS     SHA1_PREFIX_PCG_BITS
#if SHA1_PREFIX_PCG_BITS == 64
#define   ARANDOM_GEN_TYPE pcg64_random_t
#define   ARANDOM_GEN_RAND pcg64_random_r
#else // SHA1_PREFIX_PCG_BITS == 64
#define   ARANDOM_GEN_TYPE pcg32_random_t
#define   ARANDOM_GEN_RAND pcg32_random_r
#endif // SHA1_PREFIX_PCG_BITS == 64
#include "arandom-impl.h"
#undef    ARANDOM_GEN_RAND
#undef    ARANDOM_GEN_TYPE
#undef    ARANDOM_BITS
#undef    ARANDOM_NAME
#undef    ARANDOM_BASE

// struct asrandom_t

#define asrandom_t     srandom_t 
#define asrandom_init  srandom_init
#define asrandom_done  srandom_done
#define asrandom_data  srandom_data
#define asrandom_error srandom_error

#define   ARANDOM_BASE     srandom
#define   ARANDOM_NAME     asrandom
#define   ARANDOM_BITS     SHA1_PREFIX_SFMT_BITS
#if SHA1_PREFIX_SFMT_BITS == 64
#define   ARANDOM_GEN_RAND sfmt_genrand_uint64
#else // SHA1_PREFIX_SFMT_BITS == 64
#define   ARANDOM_GEN_RAND sfmt_genrand_uint32
#endif // SHA1_PREFIX_SFMT_BITS == 64
#define   ARANDOM_GEN_TYPE sfmt_t
#include "arandom-impl.h"
#undef    ARANDOM_GEN_TYPE
#undef    ARANDOM_GEN_RAND
#undef    ARANDOM_BITS
#undef    ARANDOM_NAME
#undef    ARANDOM_BASE

struct random_t
{
    union {
        // stev: basic generators
        struct drandom_t  d48;
        struct mrandom_t  mt;
        struct nrandom_t  pcg;
        struct srandom_t  sfmt;
        // stev: composed generators
        struct pdrandom_t pd48;
        struct pmrandom_t pmt;
        struct pnrandom_t ppcg;
        struct psrandom_t psfmt;
        struct adrandom_t ad48;
        struct amrandom_t amt;
        struct anrandom_t apcg;
        struct asrandom_t asfmt;
    };

    void  *impl;
    void (*error)(const void*);
    void (*done)(void*);
    void (*gen)(void*);

    struct random_data_t data;
};

#undef  RANDOM_INIT
#define RANDOM_INIT(m, n, ...)         \
    ({                                 \
        bool __r =  n ## random_init(  \
            rand->impl = &rand->m,     \
            ## __VA_ARGS__);           \
        rand->error = (void (*)(const  \
            void*)) n ## random_error; \
        rand->done = (void (*)(void*)) \
            n ## random_done;          \
        rand->gen = (void (*)(void*))  \
            n ## random_gen;           \
        rand->data = n ## random_data( \
            rand->impl);               \
        __r;                           \
    })

#undef  CASE
#define CASE(t, n)                      \
    case options_random_gen_ ## t:      \
        return RANDOM_INIT(t, n, size); \

static bool random_init(
    struct random_t* rand,
    enum options_random_gen_t gen,
    size_t size)
{
    memset(rand, 0, sizeof(*rand));

    switch (gen) {
    CASE(d48,   d);
    CASE(mt,    m);
    CASE(pcg,   n);
    CASE(sfmt,  s);
    CASE(pd48,  pd);
    CASE(pmt,   pm);
    CASE(ppcg,  pn);
    CASE(psfmt, ps);
    CASE(ad48,  ad);
    CASE(amt,   am);
    CASE(apcg,  an);
    CASE(asfmt, as);
    default:
        UNEXPECT_VAR("%d", gen);
    }
}

static void random_error(struct random_t* rand)
{
    rand->error(rand->impl);
}

static void random_done(struct random_t* rand)
{
    rand->done(rand->impl);
}

static void print_hex(
    const uchar_t* buf, size_t size)
{
    const uchar_t *p = buf, *e = p + size;

    while (p < e)
        printf("%02x", *p ++);
}

#ifdef SHA1_PREFIX_COND_PACKED

#define PRINT_PREFIX_COND(n)                   \
    do {                                       \
        printf("%2d %0" #n PRIx64 "\n", n,     \
            (uint64_t) (PREFIX_COND(v.p, n))); \
    } while (0)

static bool print_prefix_cond(void)
{
    union {
        uint64_t u;
        uchar_t p[8];
    } v = {
        .p = { 0x12, 0x34, 0x56, 0x78,
               0x9a, 0xbc, 0xde, 0xff }
    };

    fputs(" x ", stdout);
    print_hex(v.p, 8);

    printf("\n 0 %" PRIx64 "\n", v.u);

    PRINT_PREFIX_COND(1);
    PRINT_PREFIX_COND(2);
    PRINT_PREFIX_COND(3);
    PRINT_PREFIX_COND(4);
    PRINT_PREFIX_COND(5);
    PRINT_PREFIX_COND(6);
    PRINT_PREFIX_COND(7);
    PRINT_PREFIX_COND(8);
    PRINT_PREFIX_COND(9);
    PRINT_PREFIX_COND(10);
    PRINT_PREFIX_COND(11);
    PRINT_PREFIX_COND(12);
    PRINT_PREFIX_COND(13);
    PRINT_PREFIX_COND(14);
    PRINT_PREFIX_COND(15);
    PRINT_PREFIX_COND(16);

    return true;
}

#endif // SHA1_PREFIX_COND_PACKED

#ifndef SHA1_PREFIX_OPENSSL_SHA1

static bool sha1_digest(
    const struct options_t* opts)
{
    uchar_t sum[SHA_DIGEST_LENGTH];
    struct input_t input;
    SHA_CTX sha;

    if (!input_init(&input,
            opts->input, opts->input_max))
        input_error(&input);

    SHA1_Init(&sha);
    SHA1_Update(&sha, input.ptr, input.size);
    SHA1_Final(sum, &sha);

    print_hex(sum, ARRAY_SIZE(sum));
    putchar('\n');

    input_done(&input);
    return true;
}

#endif // SHA1_PREFIX_OPENSSL_SHA1

#define CASE_RANDOM_GEN_LOOP(l, t, n) \
    case n: l(t, n); break

#define RANDOM_GEN_LOOP_CASES(l, t) \
    switch (opts->width) {          \
    CASE_RANDOM_GEN_LOOP(l, t, 1);  \
    CASE_RANDOM_GEN_LOOP(l, t, 2);  \
    CASE_RANDOM_GEN_LOOP(l, t, 3);  \
    CASE_RANDOM_GEN_LOOP(l, t, 4);  \
    CASE_RANDOM_GEN_LOOP(l, t, 5);  \
    CASE_RANDOM_GEN_LOOP(l, t, 6);  \
    CASE_RANDOM_GEN_LOOP(l, t, 7);  \
    CASE_RANDOM_GEN_LOOP(l, t, 8);  \
    CASE_RANDOM_GEN_LOOP(l, t, 9);  \
    CASE_RANDOM_GEN_LOOP(l, t, 10); \
    default:                        \
        UNEXPECT_VAR(               \
            "%zu", opts->width);    \
    }                               \

#define CASE_RANDOM_GEN(l, n, t)     \
    case options_random_gen_ ## n:   \
        RANDOM_GEN_LOOP_CASES(l, t); \
        break

#define RANDOM_GEN_CASES(l)            \
    do {                               \
        switch (opts->random_gen) {    \
        CASE_RANDOM_GEN(l, d48,   d);  \
        CASE_RANDOM_GEN(l, mt,    m);  \
        CASE_RANDOM_GEN(l, pcg,   n);  \
        CASE_RANDOM_GEN(l, sfmt,  s);  \
        CASE_RANDOM_GEN(l, pd48,  pd); \
        CASE_RANDOM_GEN(l, pmt,   pm); \
        CASE_RANDOM_GEN(l, ppcg,  pn); \
        CASE_RANDOM_GEN(l, psfmt, ps); \
        CASE_RANDOM_GEN(l, ad48,  ad); \
        CASE_RANDOM_GEN(l, amt,   am); \
        CASE_RANDOM_GEN(l, apcg,  an); \
        CASE_RANDOM_GEN(l, asfmt, as); \
        default:                       \
            UNEXPECT_VAR("%d",         \
                opts->random_gen);     \
        }                              \
    } while (0)

#undef  RANDOM_GEN
#define RANDOM_GEN(t) t ## random_gen

#define STRINGS_LOOP(t, n)        \
    do {                          \
        VERIFY(                   \
            rand->gen ==          \
            (void (*)(void*))     \
            RANDOM_GEN(t));       \
        do {                      \
            RANDOM_GEN(t)(        \
                rand->impl);      \
            print_hex(            \
                rand->data.ptr,   \
                rand->data.size); \
            putchar('\n');        \
        } while (-- k);           \
    } while (0)

static bool gen_strings(
    struct random_t* rand,
    const struct options_t* opts)
{
    size_t k = opts->strings;

    RANDOM_GEN_CASES(STRINGS_LOOP);

    return true;
}

static volatile sig_atomic_t sighup_raised = 0;

static void sighup_handler(int signal UNUSED)
{
    sighup_raised = 1;
}

#define LOOP_COND_MAX(k, m) \
    if (UNLIKELY(sighup_raised || k >= m)) break
#define LOOP_COND(k, m)

#define PREFIX_LOOP_(t, n, c)             \
    do {                                  \
        VERIFY(                           \
            rand->gen ==                  \
            (void (*)(void*))             \
            RANDOM_GEN(t));               \
        b = CLOCK_START();                \
        do {                              \
            k ++;                         \
            c(k, m);                      \
            sha2 = sha;                   \
            RANDOM_GEN(t)(                \
                rand->impl);              \
            SHA1_Update(&sha2,            \
                rand->data.ptr,           \
                rand->data.size);         \
            SHA1_Final(sum, &sha2);       \
        } while (                         \
            LIKELY(PREFIX_COND(sum, n))); \
        p = PREFIX_COND(sum, n);          \
        e = CLOCK_STOP();                 \
    } while (0)

#define PREFIX_LOOP_MAX(t, n) \
        PREFIX_LOOP_(t, n, LOOP_COND_MAX)
#define PREFIX_LOOP(t, n) \
        PREFIX_LOOP_(t, n, LOOP_COND)

static bool sha1_prefix(
    struct random_t* rand,
    const struct options_t* opts)
{
    size_t k = 0, m = opts->strings;
#ifdef SHA1_PREFIX_COND_PACKED
    ALIGNED(MEM_ALIGNOF(max_align_t))
#endif
    uchar_t sum[SHA_DIGEST_LENGTH];
    struct input_t input;
    struct sigaction act;
    SHA_CTX sha, sha2;
    uint64_t b, e, c;
    bool p;

    if (!input_init(&input,
            opts->input, opts->input_max))
        input_error(&input);

    if (opts->sighup) {
        memset(&act, 0, sizeof act);
        act.sa_handler = sighup_handler;
        sigaction(SIGHUP, &act, NULL);
    }

    SHA1_Init(&sha);
    SHA1_Update(&sha, input.ptr, input.size);

    if (opts->sighup || m < SIZE_MAX)
        RANDOM_GEN_CASES(PREFIX_LOOP_MAX);
    else
        RANDOM_GEN_CASES(PREFIX_LOOP);
    c = CLOCK_SUB(e, b);

    fputs("gen ", stdout);
    options_print_random_gen_name(opts->random_gen);
    printf(" %zu %zu %.2f\ndata-hex ", k, c, (double) c / k);
    print_hex(input.ptr, input.size);

    if (!p) {
        putchar(' ');
        print_hex(rand->data.ptr, rand->data.size);
    }
    fputs("\nsha1 ", stdout);
    if (!p)
        print_hex(sum, ARRAY_SIZE(sum));
    else
        putchar('-');
    putchar('\n');

    input_done(&input);

    return !p;
}

#undef  CASE
#define CASE(n) \
    [options_action_ ## n] = n

static bool run_random(
    const struct options_t* opts)
{
    typedef bool (*action_func_t)(
        struct random_t*, const struct options_t*);
    static const action_func_t actions[] = {
        CASE(gen_strings),
        CASE(sha1_prefix),
    };
    action_func_t act = ARRAY_NULL_ELEM(
        actions, opts->action);
    struct random_t rand;
    bool r;

    if (!random_init(&rand,
            opts->random_gen, opts->length))
        random_error(&rand);

    ASSERT(act != NULL);
    r = act(&rand, opts);

    random_done(&rand);
    return r;
}

int main(int argc, char* argv[])
{
    const struct options_t* opts =
        options(argc, argv);

    return !(
#ifdef SHA1_PREFIX_COND_PACKED
        opts->action ==
        options_action_prefix_cond ?
        print_prefix_cond() :
#endif // SHA1_PREFIX_COND_PACKED
#ifndef SHA1_PREFIX_OPENSSL_SHA1
        opts->action ==
        options_action_sha1_digest ?
        sha1_digest(opts) :
#endif // SHA1_PREFIX_OPENSSL_SHA1
        run_random(opts)
    );
}


