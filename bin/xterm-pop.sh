#!/bin/bash

if [[ -n "$1" ]] ; then
    /usr/bin/xterm -class Xpop -geometry 96x32-1-1 -e "echo $*; $HOME/bin/$* ; sleep 1"
fi

