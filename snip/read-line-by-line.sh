#!/bin/sh

IFS=$'\n'
for line in `cat`; do
    echo $line
done

exit 0

