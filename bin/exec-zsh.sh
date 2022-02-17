#!/bin/sh

export EDITOR=vim SHELL=/usr/bin/zsh LS_COLORS="$(vivid generate molokai)"
exec $SHELL --login -i

