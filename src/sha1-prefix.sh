#!/bin/bash

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

set -o pipefail
shopt -s extglob
source commands.sh

prog="$0"
self="$(basename "$prog")"
base="${self%.*}"

b=-     # batch:   [-+] or \+\+ or \+? followed by
        #            a comma-separated list of
        #            [0-9]+:[0-9]+[smhd]
l=0     # label:   [0-9]+
r=0     # repeat:  \+?[0-9]+
t=-     # timeout: [-+]|\+?[0-9]+[smhd]?
u=-     # cpu affinity: -|[0-3]
i='+16' # input: \+[0-9]+|([0-9a-fA-F][0-9a-fA-F])+
g='mt'  # pass `-g$g' to './sha1-prefix'
n=-     # pass `-n$n' to './sha1-prefix'
s=16    # pass `-s$s' to './sha1-prefix'
w=3     # pass `-w$w' to './sha1-prefix'

uex='[0-3]'
nex='+([0-9])'
tex="$nex?([smhd])"
bex="@(?(+)+|?(+)$nex:$tex*(,$nex:$tex))"
gex='?([pa])@(d48|mt|pcg|sfmt)'
iex="@(+$nex|+([0-9a-fA-F][0-9a-fA-F]))"

parse-opts()
{
    local opt
    local opts=":b:g:i:l:n:r:s:t:u:w:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        case "$opt" in
            [lsw])
                [[ "$OPTARG" == $nex ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            b)	[[ "$OPTARG" == @(-|$bex) ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            g)	[[ "$OPTARG" == @(+|$gex) ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            i)	[[ "$OPTARG" == $iex ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            n)	[[ "$OPTARG" == @(-|$nex) ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            r)	[[ "$OPTARG" == ?([+])$nex ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            t)	[[ "$OPTARG" == @([-+]|?(+)$tex) ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            u)	[[ "$OPTARG" == @(-|$uex) ]] || {
                    error -i
                    return 1
                }
                optarg
                ;;
            *)	error -g
                return 1
        esac
    done
    return 0
}

loop() 
{ 
    local c="$prog $@"
    # stev: assuming all options have arg:
    c="${c//?( )-g?( )[+]?( )/ }"
    c="${c%%?( )}"

    local g
    for g in {,p,a}{d48,mt,pcg,sfmt}; do
        eval "$c -g $g"
        sleep 1
    done

    return 0
}

batch()
{
    local c="$prog $@"
    # stev: assuming all options have arg:
    c="${c//?( )-b?( )$bex?( )/ }"
    c="${c%%?( )}"

    local p=''
    [ "${#b}" -gt 1 -a "${b:0:1}" == '+' ] && {
        b="${b:1}"
        p='+'
    }

    #!!! [ "$b" == '+' ] &&
    #!!! b='15:1m,7:2m,3:4m,2:8m,2:16m,1:30m'
    #!!! #    15 + 14 + 12 + 16 + 32 + 30 = 119m = 1h59m

    #!!! [ "$b" == '+' ] &&
    #!!! b='10:2m,5:4m,2:8m,2:15m,1:33m'
    #!!! #    20 + 20 + 16 + 30 + 33 = 119m = 1h59m

    #!!! [ "$b" == '+' ] &&
    #!!! b='10:2m,5:4m,2:8m,1:63m'
    #!!! #    20 + 20 + 16 + 63 = 119m = 1h59m

    [ "$b" == '+' ] &&
    b='10:2m,5:4m,1:16m,1:63m'
    #    20 + 20 + 16 + 63 = 119m = 1h59m

    local a

    time \
    for a in ${b//,/ }; do
        eval "$c -r +${a/:/ -t${p:+ $p}}" &&
        break
    done

    return $?
}

repeat() 
{ 
    local c="$prog $@"
    # stev: assuming all options have arg:
    c="${c//?( )-r?( )?(+)$nex?( )/ }"
    c="${c%%?( )}"

    local a=''
    [ "${r:0:1}" == '+' ] && a='+'

    time \
    (
        while [ "$((r --))" -gt 0 ]; do
            eval "$c -l$((r + 1))" && {
                [ -n "$a" ] &&
                exit 0
            }
            [ "$r" -gt 0 ] &&
            sleep 1
        done
        exit 1
    ) 2>&1 |{
        stdbuf -oL -eL tee -a "$log" ||
        true
    }

    return $?
}

time-out() 
{ 
    local c="$prog $@"
    # stev: assuming all options have arg:
    c="${c//?( )-t?( )?(+)$tex?( )/ }"
    c="${c//?( )-l?( )$nex?( )/ }"
    c="${c%%?( )}"

    local s=''
    [ "${t:0:1}" == '+' ] && {
        t="${t:1}"
        s='-s HUP'
        c+=' -t+'
    }

    local L="$l"
    [ "$L" -gt 0 ] &&
    L=" #$L/$t" ||
    L=''

    echo "$ $c$L"

    timeout $s $t $c
    local r=$?

    [ "$r" -eq 124 ] && { 
        echo >&2 "command timed out: $c$L"
        return 1
    }
    return $r
}

sha1-prefix()
{
    local e=''
    [ "$t" == '+' ] && e='-e'

    if [ "$u" != '-' ]; then
        taskset -c $u \
        ./sha1-prefix "$@" $e
    else
        ./sha1-prefix "$@" $e
    fi
}

parse-opts "$@" ||
exit 1

if [ "$b" != '-' ]; then
    TIMEFORMAT='batch'
elif [ "$r" -gt 0 ]; then
    TIMEFORMAT='repeat'
else
    TIMEFORMAT='exec'
fi
TIMEFORMAT+=' %lR'

[ "$u" != '-' ] &&
log="$base$u.log" ||
log="$base.log"

[ "${i:0:1}" == '+' ] && {
    i="$(head -c${i:1} /dev/urandom|
         hexdump -ve '1/1 "%02x"')" && \
    [ "${i:0:1}" != '+' ] || {
        error "inner command failed: head|hexdump"
        exit 1
    }
    i="$@ -i $i"
    # stev: assuming all options have arg:
    i="${i//?( )-i?( )+$nex?( )/ }"

    set -- $i
    i="${i##*-i }"
}

[[ "$g" == '+' || "$r" -gt 0 || "$t" != '-' ]] &&
echo "$ $prog $@" >> "$log"

if [ "$g" == '+' ]; then
    loop "$@"
elif [ "$b" != '-' ]; then
    batch "$@"
elif [ "$r" -gt 0 ]; then
    repeat "$@"
elif [[ "$t" != [-+] ]]; then
    time-out "$@"
else
    time \
    xxd -r -p <<< "$i"|
    sha1-prefix -g$g -n$n -s$s -w$w
fi


