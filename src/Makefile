# Copyright (C) 2019  Stefan Vargyas
# 
# This file is part of Sha1-Prefix.
# 
# Sha1-Prefix is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Sha1-Prefix is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Sha1-Prefix.  If not, see <http://www.gnu.org/licenses/>.

.PHONY:: default clean allclean all depend

default: all

PROGRAM := sha1-prefix

JSON_TYPE_HOME := ../lib/json-type
JSON_TYPE_LIB  := ${JSON_TYPE_HOME}/lib

ifdef OPENSSL_SHA1
CRYPTO_HOME := /usr
SHA1_INCLUDE := ${CRYPTO_HOME}/include
SHA1_LIB := ${CRYPTO_HOME}/lib64
SHA1_ARH := libcrypto.a
else
SHA1_HOME := ../lib/sha1
SHA1_INCLUDE := ${SHA1_HOME}
SHA1_LIB := ${SHA1_HOME}
SHA1_ARH := sha1.a
endif

MT_HOME := ../lib/mt
PCG_HOME := /usr/local
SFMT_HOME := sfmt

SRCS := ${JSON_TYPE_LIB}/file-buf.c \
        ${JSON_TYPE_LIB}/su-size.c \
        ${SFMT_HOME}/SFMT.c \
        sha1-prefix.c

BIN := ${PROGRAM}

# GCC parameters

GCC := gcc
GCC_STD := gnu11

CFLAGS := -Wall -Wextra -std=${GCC_STD} \
          -g -I. -I${JSON_TYPE_HOME} -I${JSON_TYPE_LIB} -I${MT_HOME} \
          -I${SHA1_INCLUDE} -I${PCG_HOME}/include -I${SFMT_HOME} \
          -fdata-sections -ffunction-sections -Wl,--gc-sections \
          -fno-strict-aliasing -fvisibility=hidden -march=native \
          -DSFMT_MEXP=19937 -DHAVE_SSE2 -msse2 \
          -DPROGRAM=${PROGRAM} \
          -DJSON_DEBUG \
          -DDEBUG

LDFLAGS := -L ${SHA1_LIB} -l:${SHA1_ARH} \
           -L ${PCG_HOME}/lib -l:libpcg_random.a

ifdef OPENSSL_SHA1
CFLAGS += -DSHA1_PREFIX_OPENSSL_SHA1
endif

ifdef PCG_BITS
ifneq ($(words ${PCG_BITS}),1)
$(error invalid PCG_BITS='${PCG_BITS}')
endif
ifneq ($(filter-out 32 64,${PCG_BITS}),)
$(error invalid PCG_BITS='${PCG_BITS}')
endif
else
PCG_BITS := 64
endif
CFLAGS += -DSHA1_PREFIX_PCG_BITS=${PCG_BITS}

ifdef SFMT_BITS
ifneq ($(words ${SFMT_BITS}),1)
$(error invalid SFMT_BITS='${SFMT_BITS}')
endif
ifneq ($(filter-out 32 64,${SFMT_BITS}),)
$(error invalid SFMT_BITS='${SFMT_BITS}')
endif
else
SFMT_BITS := 32
endif
CFLAGS += -DSHA1_PREFIX_SFMT_BITS=${SFMT_BITS}

ifdef OPT
ifneq ($(words ${OPT}),1)
$(error invalid OPT='${OPT}')
endif
ifneq ($(filter-out 0 1 2 3,${OPT}),)
$(error invalid OPT='${OPT}')
endif
else
OPT := 3
endif
CFLAGS += -O$(strip ${OPT})
LDFLAGS += -O$(strip ${OPT})

# dependency rules

ifeq (.depend, $(wildcard .depend))
include .depend
endif

# building rules

${BIN}:
	${GCC} ${CFLAGS} ${SRCS} ${LDFLAGS} -o $@

# main targets

all: ${BIN}

preprocess:
	${GCC} ${CFLAGS} sha1-prefix.c -o - -E -P 2>&1

clean:
	rm -f *~

allclean: clean
	rm -f ${BIN}

depend:
	${GCC} ${CFLAGS} -c ${SRCS} -MM| \
    sed -r 's/^[^ \t]+\.o:/${BIN}:/' > .depend


