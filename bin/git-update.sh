#!/bin/sh

## find ggez/ state_machine_future/ -type d -name .git -exec update.git.sh '{}'/.. \;

if [ "`basename $1`" = ".git" ] ; then
    cd "$1"/.. || exit 1
else
    cd "$1" || exit 1
fi

echo "### $1: `git remote get-url origin`"

git pull && git submodule update --init --recursive

