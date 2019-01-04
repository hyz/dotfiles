#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

if [ $# -eq 1 -a -f "$1" ] ; then
    if [ -r "$1" ] ; then
        if which bat >/dev/null ; then
            bat "$1"
        else
            cat "$1"
        fi
    fi
    exit
fi

if [ $# -ge 1 ] ; then
    for x in $* ; do
        if [ -d "$x" ] ; then
            /bin/bash $0 `find "$x" -maxdepth 1 -type f -iname "README*"`
        elif [ -r "$x" ] ; then
            /bin/bash $0 "$x"
        fi
    done
else
    /bin/bash $0 `find * -maxdepth 1 -type f -iname "README*"`
fi

