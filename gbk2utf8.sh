#!/bin/sh

for x in $*; do
    iconv -f gbk -t utf-8 "$x" > "$x.txt"
done

