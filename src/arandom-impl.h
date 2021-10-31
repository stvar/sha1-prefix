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

#ifndef ARANDOM_NAME
#error  ARANDOM_NAME is not defined
#endif

#ifndef ARANDOM_BITS
#error  ARANDOM_BITS is not defined
#elif   ARANDOM_BITS != 32 && ARANDOM_BITS != 64
#error  ARANDOM_BITS is not 32 neither 64
#endif

#ifndef ARANDOM_GEN_RAND
#error  ARANDOM_GEN_RAND is not defined
#endif

#define ARANDOM_MAKE_BITS_(b, n) ARANDOM_ ## n ## b
#define ARANDOM_MAKE_BITS(b, n)  ARANDOM_MAKE_BITS_(b, n)

#define ARANDOM_RNG   ARANDOM_MAKE_BITS(ARANDOM_BITS, RNG)
#define ARANDOM_MIN   ARANDOM_MAKE_BITS(ARANDOM_BITS, MIN)

#define ARANDOM_RNG32 0x3f3f3f3fu
#define ARANDOM_MIN32 0x3b3b3b3bu

#define ARANDOM_RNG64 0x3f3f3f3f3f3f3f3ful
#define ARANDOM_MIN64 0x3b3b3b3b3b3b3b3bul

#define ARANDOM_GEN_RAND_(g) \
    (                        \
       (ARANDOM_GEN_RAND(g)  \
            & ARANDOM_RNG)   \
            + ARANDOM_MIN    \
    )

#if       ARANDOM_BITS == 64
#define   RANDOM_BITS     64
#else  // ARANDOM_BITS == 64
#define   RANDOM_BITS     32
#endif // ARANDOM_BITS == 64
#define   RANDOM_BASE     ARANDOM_BASE
#define   RANDOM_NAME     ARANDOM_NAME
#define   RANDOM_GEN_TYPE ARANDOM_GEN_TYPE
#define   RANDOM_GEN_RAND ARANDOM_GEN_RAND_

#define   RANDOM_NEED_GEN_ONLY
#include "random-impl.h"
#undef    RANDOM_NEED_GEN_ONLY

#undef    RANDOM_GEN_RAND
#undef    RANDOM_GEN_TYPE
#undef    RANDOM_NAME
#undef    RANDOM_BASE
#undef    RANDOM_BITS


