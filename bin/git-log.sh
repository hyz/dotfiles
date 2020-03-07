#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

if [ $# -eq 0 ] ; then
    exec /bin/bash $0 .
fi

cd "$1" || die "cd: $1"
while [ ! -d .git ] ; do
    cd .. || die "not found: .git"
done

#test -d .git || die "not found: .git"
echo "+++ `pwd`: `git remote get-url origin`"
git log
echo "--- `pwd`"
echo

