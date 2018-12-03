#!/bin/bash

die() {
    echo "$*" >&2
    eixt 1
}

if [ $# -eq 0 ] ; then
    test -d .git || die ".git not-exist"
    echo "### `pwd`: `git remote get-url origin`"
    git pull && git submodule update --init --recursive
    exit
fi

find $* -type d -name .git -prune \
    -exec /bin/bash -c "cd '{}'/.. && /bin/bash $0" \;

