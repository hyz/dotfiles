#!/bin/sh

## find ggez/ state_machine_future/ -type d -name .git -exec update.git.sh '{}'/.. \;

[ -d "$1"/.git ] || exit 1
echo "### $1"

cd "$1" \
    && git pull \
    && git submodule update --recursive

