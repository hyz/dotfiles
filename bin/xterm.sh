#!/bin/sh

export LS_COLORS="$(vivid generate molokai)"
export SHELL=/usr/bin/zsh
exec $SHELL --login -i

