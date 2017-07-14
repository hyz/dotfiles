#!/bin/sh

fifo=$2

case "$1" in
"read")
    exec 0<$fifo
    while true ; do
        if dd count=1 bs=5 ; then
            sleep 1
        else
            echo "$?" >&2;
        fi
        #sleep 5
    done
    ;;
"write")
    exec 1>$fifo
    while true ; do
        if dd if=/dev/zero count=10 bs=$(($RANDOM % 4 + 1)) ; then
            sleep 1
        else
            echo "$?" >&2;
            sleep 2
        fi
    done
    ;;
esac

