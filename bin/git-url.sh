#!/bin/sh

if [ "`basename $1`" = ".git" ] ; then
    cd "$1"/.. || exit 1
else
    cd "$1" || exit 1
fi
git remote get-url origin

