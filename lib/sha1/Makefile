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

LIBRARY := sha1

ifdef SHA1_ASM
ifneq ($(words ${SHA1_ASM}),1)
$(error invalid SHA1_ASM='${SHA1_ASM}')
endif
ifneq ($(filter-out no yes,${SHA1_ASM}),)
$(error invalid SHA1_ASM='${SHA1_ASM}')
endif
else
SHA1_ASM := yes
endif

ifeq (${SHA1_ASM},yes)
ASMS := sha1-x86_64.s x86_64cpuid.s
SRCS := init.c
endif

SRCS += sha1.c
OBJS := $(patsubst %.c,%.o, ${SRCS}) \
        $(patsubst %.s,%.o, ${ASMS})

BIN := ${LIBRARY}.a

# GCC parameters

GCC := gcc
GCC_STD := gnu11

GAS := as
GAR := gcc-ar
GRANLIB := ranlib

CFLAGS := -Wall -Wextra -std=${GCC_STD} -I. \
          -fdata-sections -ffunction-sections -Wl,--gc-sections \
          -fvisibility=hidden -march=native -g

ASFLAGS := --64

ifeq (${SHA1_ASM},yes)
CFLAGS += -DSHA1_ASM
endif

ifdef SHA1_BLOCK_DATA_ORDER
ifneq ($(words ${SHA1_BLOCK_DATA_ORDER}),1)
$(error invalid SHA1_BLOCK_DATA_ORDER='${SHA1_BLOCK_DATA_ORDER}')
endif
ifneq ($(filter-out avx avx2 ssse3 shaext,${SHA1_BLOCK_DATA_ORDER}),)
$(error invalid SHA1_BLOCK_DATA_ORDER='${SHA1_BLOCK_DATA_ORDER}')
endif
else
SHA1_BLOCK_DATA_ORDER := avx
endif
CFLAGS += -DSHA1_BLOCK_DATA_ORDER=${SHA1_BLOCK_DATA_ORDER}


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

# dependency rules

ifeq (.depend, $(wildcard .depend))
include .depend
endif

# building rules

%.o: %.s
	${GAS} ${ASFLAGS} $< -o $@ 

%.o: %.c
	${GCC} ${CFLAGS} -c $< -o $@ 

${BIN}: ${OBJS}
	${GAR} -rc $@ $^
	${GRANLIB} $@

# main targets

all: ${BIN}

clean:
	rm -f *~ *.o

allclean: clean
	rm -f ${BIN}

depend:
	${GCC} ${CFLAGS} -c ${SRCS} -MM| \
    sed -r 's/^[^ \t]+\.o:/${BIN}:/' > .depend


