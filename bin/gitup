#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

if [ $# -eq 0 ] ; then
    test -d .git || die ".git not-exist"
    echo "+++ `pwd`: `git remote get-url origin`"
    git pull && git submodule update --init --recursive
    echo "--- `pwd`"
    echo
    exit
fi

if [ $# -eq 1 -a -f "$1" ] ; then
    exec /bin/bash $0 "`dirname $1`"
fi

find $* -type d -name .git -prune -exec /bin/bash -c "cd '{}'/.. && /bin/bash $0" \;

