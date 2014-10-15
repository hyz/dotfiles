#!/bin/sh


Closed=false
until $Closed; do
    read line || Closed=true ; [ -n "$line" ] || continue

    echo "$line"
done

### ###
# read line
# echo "$line"
# while read line || [[ -n "$line" ]] ; do
#     echo "$line"
# done

### ###
# IFS=$'\n'
# for line in `cat`; do
#     echo $line
# done

exit 0

