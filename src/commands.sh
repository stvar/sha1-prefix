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

usage()
{
    echo "usage: $1 [$(sed 's/^://;s/-:$/\x0/;s/[^:]/|-\0/g;s/:/ <arg>/g;s/^|//;s/\x0/-<long>/' <<< "$2")]"
}

quote()
{
    local __n__
    local __v__

    [ -z "$1" -o "$1" == "__n__" -o "$1" == "__v__" ] &&
    return 1

    printf -v __n__ '%q' "$1"
    eval __v__="\"\$$__n__\""
    #!!! echo "!!! 0 __v__='$__v__'"
    test -z "$__v__" && return 0
    printf -v __v__ '%q' "$__v__"
    #!!! echo "!!! 1 __v__='$__v__'"
    printf -v __v__ '%q' "$__v__"  # double quote
    #!!! echo "!!! 2 __v__='$__v__'"
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optopt()
{
    local __n__="${1:-$opt}"       #!!!NONLOCAL
    local __v__=''
    test -n "$__n__" &&
    printf -v __v__ '%q' "$__n__"  # paranoia
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optarg()
{
    local __n__="${1:-$opt}"       #!!!NONLOCAL
    local __v__=''
    test -n "$OPTARG" &&
    printf -v __v__ '%q' "$OPTARG" #!!!NONLOCAL
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optact()
{
    local __v__="${1:-$opt}"       #!!!NONLOCAL
    printf -v __v__ '%q' "$__v__"  # paranoia
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "act=$__v__"
}

optlong()
{
    local a="$1"

    if [ "$a" == '-' ]; then
        if [ -z "$OPT" ]; then                                      #!!!NONLOCAL
            local A="${OPTARG%%=*}"                                 #!!!NONLOCAL
            OPT="-$opt$A"                                           #!!!NONLOCAL
            OPTN="${OPTARG:$((${#A})):1}"                           #!!!NONLOCAL
            OPTARG="${OPTARG:$((${#A} + 1))}"                       #!!!NONLOCAL
        else
            OPT="--$OPT"                                            #!!!NONLOCAL
        fi
    elif [ "$opt" == '-' -o \( -n "$a" -a -z "$OPT" \) ]; then      #!!!NONLOCAL
        OPT="${OPTARG%%=*}"                                         #!!!NONLOCAL
        OPTN="${OPTARG:$((${#OPT})):1}"                             #!!!NONLOCAL
        OPTARG="${OPTARG:$((${#OPT} + 1))}"                         #!!!NONLOCAL
        [ -n "$a" ] && OPT="$a-$OPT"                                #!!!NONLOCAL
    elif [ -z "$a" ]; then                                          #!!!NONLOCAL
        OPT=''                                                      #!!!NONLOCAL
        OPTN=''                                                     #!!!NONLOCAL
    fi
}

optlongchkarg()
{
    test -z "$OPT" &&                               #!!!NONLOCAL
    return 0

    [[ "$opt" == [a-zA-Z] ]] || {                   #!!!NONLOCAL
        error "internal: invalid opt name '$opt'"   #!!!NONLOCAL
        return 1
    }

    local r="^:[^$opt]*$opt(.)"
    [[ "$opts" =~ $r ]]
    local m="$?"

    local s
    if [ "$m" -eq 0 ]; then
        s="${BASH_REMATCH[1]}"
    elif [ "$m" -eq 1 ]; then
        error "internal: opt '$opt' not in '$opts'" #!!!NONLOCAL
        return 1
    elif [ "$m" -eq "2" ]; then
        error "internal: invalid regex: $r"
        return 1
    else
        error "internal: unexpected regex match result: $m: ${BASH_REMATCH[@]}"
        return 1
    fi

    if [ "$s" == ':' ]; then
        test -z "$OPTN" && {                        #!!!NONLOCAL
            error --long -a
            return 1
        }
    else
        test -n "$OPTN" && {                        #!!!NONLOCAL
            error --long -d
            return 1
        }
    fi
    return 0
}

error()
{
    local __self__="$self"     #!!!NONLOCAL
    local __help__="$help"     #!!!NONLOCAL
    local __OPTARG__="$OPTARG" #!!!NONLOCAL
    local __opts__="$opts"     #!!!NONLOCAL
    local __opt__="$opt"       #!!!NONLOCAL
    local __OPT__="$OPT"       #!!!NONLOCAL

    local self="error"

    # actions: \
    #  a:argument for option -$OPTARG not found|
    #  o:when $OPTARG != '?': invalid command line option -$OPTARG, or, \
    #    otherwise, usage|
    #  i:invalid argument '$OPTARG' for option -$opt|
    #  d:option '$OPTARG' does not take arguments|
    #  e:error message|
    #  w:warning message|
    #  u:unexpected option -$opt|
    #  g:when $opt == ':': equivalent with 'a', \
    #    when $opt == '?': equivalent with 'o', \
    #    when $opt is anything else: equivalent with 'u'

    local act="e"
    local A="$__OPTARG__" # $OPTARG
    local h="$__help__"   # $help
    local m=""            # error msg
    local O="$__opts__"   # $opts
    local P="$__opt__"    # $opt
    local L="$__OPT__"    # $OPT
    local S="$__self__"   # $self

    local long=''         # short/long opts (default)

    #!!! echo "!!! A='$A'"
    #!!! echo "!!! O='$O'"
    #!!! echo "!!! P='$P'"
    #!!! echo "!!! L='$L'"
    #!!! echo "!!! S='$S'"

    local opt
    local opts=":aA:degh:iL:m:oO:P:S:uw-:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        case "$opt" in
            [adeiouwg])
                act="$opt"
                ;;
            #[])
            #	optopt
            #	;;
            [AhLmOPS])
                optarg
                ;;
            \:)	echo "$self: error: argument for option -$OPTARG not found" >&2
                return 1
                ;;
            \?)	if [ "$OPTARG" != "?" ]; then
                    echo "$self: error: invalid command line option -$OPTARG" >&2
                else
                    echo "$self: $(usage $self $opts)"
                fi
                return 1
                ;;
            -)	case "$OPTARG" in
                    long|long-opts)
                        long='l' ;;
                    short|short-opts)
                        long='' ;;
                    *)	echo "$self: error: invalid command line option --$OPTARG" >&2
                        return 1
                        ;;
                esac
                ;;
            *)	echo "$self: error: unexpected option -$OPTARG" >&2
                return 1
                ;;
        esac
    done
    #!!! echo "!!! A='$A'"
    #!!! echo "!!! O='$O'"
    #!!! echo "!!! P='$P'"
    #!!! echo "!!! L='$L'"
    #!!! echo "!!! S='$S'"
    shift $((OPTIND - 1))
    test -n "$1" && m="$1"
    local f="2"
    if [ "$act" == "g" ]; then
        if [ "$P" == ":" ]; then
            [ "$A" != "-" ] &&
            act="a" ||
            act="o"
        elif [ "$P" == "?" ]; then
            act="o"
        else 
            act="u"
        fi
    fi
    local o=''
    if [ -n "$long" -a -n "$L" ]; then
        test "${L:0:1}" != '-' && o+='--'
        o+="$L"
    elif [[ "$act" == [aod] ]]; then
        o="-$A"
    elif [[ "$act" == [iu] ]]; then
        o="-$P"
    fi
    case "$act" in
        a)	m="argument for option $o not found"
            ;;
        o)	if [ "$A" != "?" ]; then
                m="invalid command line option $o"
            else
                act="h"
                m="$(usage $S $O)"
                f="1"
            fi
            ;;
        i)	m="invalid argument for $o: '$A'"
            ;;
        u)	m="unexpected option $o"
            ;;
        d)	m="option $o does not take arguments"
            ;;
        *)	# [ew]
            if [ "$#" -ge "2" ]; then
                S="$1"
                m="$2"
            elif [ "$#" -ge "1" ]; then
                m="$1"
            fi
            ;;
    esac
    if [ "$act" == "w" ]; then
        m="warning${m:+: $m}"
    elif [ "$act" != "h" ]; then
        m="error${m:+: $m}"
    fi
    if [ -z "$S" -o "$S" == "-" ]; then
        printf "%s\n" "$m" >&$f
    else
        printf "%s: %s\n" "$S" "$m" >&$f
    fi
    if [ "$act" == "h" ]; then
        test -n "$1" && h="$1"
        test -n "$h" &&
        printf "%s\n" "$h" >&$f
    fi
    return $f
}

runcmds()
{
    local self='runcmds'

    local open="\x1b\x5b\x30\x31"
    local close="\x1b\x5b\x30\x6d"
    local red="$open\x3b\x33\x31\x6d"
    local cyan="$open\x3b\x33\x36\x6d"

    local x='eval'
    local i=''     # the input commands are interactive
    local e=''     # echo uncolored command before executing it
    local E=''     # echo red colored command before executing it
    local C="$red" # echo cyan colored command before executing it
    local s=''     # seconds to sleep between commands

    local opt
    local opts=":CdeEis:x"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        case "$opt" in
            d)	x="echo"
                ;;
            x)	x="eval"
                ;;
            [eEi])
                optopt
                ;;
            C)	C="$cyan"
                E='E'
                ;;
            s)	if [ "$OPTARG" == '-' ]; then
                    OPTARG=''
                elif [[ "$OPTARG" != +([0-9]) ]]; then
                    error -i
                    return 1
                fi
                optarg
                ;;
            *)	error -g
                return 1
                ;;
        esac
    done
    shift $((OPTIND - 1))

    [ -n "$E" ] && e='e'

    local c
    local n=0
    while read -r c; do
        [ "$x" == 'echo' ] ||
        [ -z "$s" -o "$((n ++))" -eq 0 ] ||
        sleep $s

        [ -n "$E" ] && echo -ne "$C"
        [ -n "$e" ] && echo ${E:+-n} "$ $c"
        [ -n "$E" ] && echo -e "$close"

        $x "$c${i:+ < /dev/tty}"
    done
}

openssl-sha1-diff()
{
    local self="openssl-sha1-diff"
    local trgx='@(openssl-sha1)'
    local deff='lib/$t-files.txt'
    local defs="$HOME/openssl"
    local deft='openssl-sha1'

    local x="eval"
    local b=""          # pass '-b|--ignore-space-change' to diff (--ignore-space-change)
    local f="+"         # target's file list file name ('-' means stdin, the default is '$home/lib/$target-files.txt') (--file-list=FILE)
    local h="+"         # home dir (default: '.') (--home=DIR)
    local s="+"         # target's source tree directory (default: '$HOME/openssl') (--source-dir=DIR)
    local t="+"         # target name: 'openssl-sha1' (--target=NAME|--openssl-sha1)
    local u=""          # pass '-u|--unified=NUM' to diff (--unified=NUM)

    local opt
    local OPT
    local OPTN
    local opts=":bdf:h:s:t:u:x-:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        # discriminate long options
        optlong

        # translate long options to short ones
        test -n "$OPT" &&
        case "$OPT" in
            ignore-space-change)
                opt='b' ;;
            file-list)
                opt='f' ;;
            home)
                opt='h' ;;
            source-dir)
                opt='s' ;;
            target|$trgx)
                opt='t' ;;
            unified)
                opt='u' ;;
            *)	error --long -o
                return 1
                ;;
        esac

        # check long option argument
        [[ "$opt" == [t] ]] ||
        optlongchkarg ||
        return 1

        # handle short options
        case "$opt" in
            d)	x="echo"
                ;;
            x)	x="eval"
                ;;
            [fhs])
                optarg
                ;;
            [b])
                optopt
                ;;
            t)	[[ -n "$OPT" || "$OPTARG" == $trgx ]] || {
                    error -i
                    return 1
                }
                optlong -

                [[ "${OPT:2}" == @(target|$trgx) ]] || {
                    error --long -o
                    return 1
                }
                case "${OPT:2}" in
                    target)
                        [ -n "$OPTN" ] || {
                            error --long -a
                            return 1
                        }
                        [[ "$OPTARG" == $trgx ]] || {
                            error --long -i
                            return 1
                        }
                        t="$OPTARG"
                        ;;
                    $trgx)
                        [ -z "$OPTN" ] || {
                            error --long -d
                            return 1
                        }
                        t="${OPT:2}"
                        ;;
                    *)	error "internal: unexpected OPT='$OPT'"
                        return 1
                        ;;
                esac
                ;;
            u)	[[ "$OPTARG" == +([0-9]) ]] || {
                    error --long -i
                    return 1
                }
                optarg
                ;;
            *)	error --long -g
                return 1
                ;;
        esac
    done
    shift $((OPTIND - 1))

    [ -z "$h" ] && {
        error "home dir cannot be null"
        return 1
    }
    [ "$h" == '+' ] && h='.'
    [ ! -d "$h" ] && {
        error "home dir '$h' not found"
        test "$x" == "eval" && return 1
    }

    [ "$t" == '+' ] && t="$deft"

    [ -n "$s" ] || {
        error "source tree directory cannot be null"
        return 1
    }
    [ "$s" == '+' ] && s="$defs"
    [ -d "$s" ] || {
        error "source tree directory '$f' not found"
        return 1
    }
    quote s

    [ -n "$f" ] || {
        error "file list file name cannot be null"
        return 1
    }
    [ "$f" == '-' ] && f=""
    [ "$f" == '+' ] && {
        eval f="\"$deff\""
        f="$h/$f"
    }
    [ -z "$f" -o -f "$f" ] || {
        error "file list file '$f' not found"
        return 1
    }
    quote f

    local s2='
    /^#|^\s*$/b
    /^\s*([^ \t\v\f\r]+)\s+([^ \t\v\f\r]+)\s*$/!b
    s||diff -'"$b"'u'"$u"' '"$s"'/\1 '"$h"'/lib/sha1/\2|p'

    c="\
sed -nr '$s2'${f:+ \\
$f}|
runcmds"

    $x "$c"
}

