#ifndef __MT19937_IMPL_H
#define __MT19937_IMPL_H

/* 
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using mt19937_init_genrand(seed)  
   or mt19937_init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#ifndef MT19937_UINT_TYPE
#define MT19937_UINT_TYPE uint_fast32_t
#endif

typedef MT19937_UINT_TYPE mt19937_uint_t;

/* Period parameters */  
#define MT19937_N 624
#define MT19937_M 397
#define MT19937_MATRIX_A   0x9908b0dfUL /* constant vector a */
#define MT19937_UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define MT19937_LOWER_MASK 0x7fffffffUL /* least significant r bits */

struct mt19937_t
{
    int mti;                      /* mti==N+1 means mt[N] is not initialized */
    mt19937_uint_t mt[MT19937_N]; /* the array for the state vector  */
};

/* initializes mt[N] with a seed */
static inline void mt19937_init_genrand(
    struct mt19937_t* m, mt19937_uint_t s)
{
    enum { N = MT19937_N };
    mt19937_uint_t* mt = m->mt;
    int mti = m->mti;

    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
        (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }

    m->mti = mti;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
static inline void mt19937_init_by_array(
    struct mt19937_t* m, mt19937_uint_t init_key[], int key_length)
{
    enum { N = MT19937_N };
    mt19937_uint_t* mt = m->mt;
    int mti = m->mti;
    int i, j, k;

    mt19937_init_genrand(m, 19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 

    m->mti = mti;
}

/* generates a random number on [0,0xffffffff]-interval */
static inline mt19937_uint_t mt19937_genrand_int32(
    struct mt19937_t* m)
{
    enum {
        N = MT19937_N,  M = MT19937_M,
        MATRIX_A = MT19937_MATRIX_A,
        UPPER_MASK = MT19937_UPPER_MASK,
        LOWER_MASK = MT19937_LOWER_MASK,
    };
    mt19937_uint_t* mt = m->mt;
    int mti = m->mti;
    mt19937_uint_t y;
    static mt19937_uint_t mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

#if 0 // stev: we make sure to call mt19937_init_genrand before mt19937_genrand_int32
        if (mti == N+1)   /* if mt19937_init_genrand() has not been called, */
            mt19937_init_genrand(5489UL); /* a default initial seed is used */
#endif

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    m->mti = mti;
    return y;
}

#endif // __MT19937_IMPL_H


