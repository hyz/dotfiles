#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

view() {
    [ -r "$1" ] || die "!readable: '$1'"
    if which bat >/dev/null ; then
        bat "$1"
    else
        cat "$1"
    fi
}

if [ $# -eq 0 ] ; then
    cnt=0 exec /bin/bash $0 .
fi

if [ $# -eq 1 ] ; then
    if [ -f "$1" ]; then
        view $1
        exit
    fi

    cnt=0
    for x in `find $* -maxdepth 1 -type f -iname "README*"` ; do
        view $x
        cnt=$(( $cnt + 1 ))
    done

    if [ $cnt -eq 0 ]; then
        lsd $*
    fi

    exit
fi

[ $# -gt 1 ] || die "should not reach"

for x in $* ; do
    cnt=0 /bin/bash $0 $x
done

