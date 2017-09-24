#!/bin/sh

echo -n "Search: "
while read sym ; do
    if [ -n "$sym" ] ; then
        clear
        xsv fmt -d'\t' a.tsv |xsv select 存货编码,现存数量,可用数量,规格型号,存货名称 |xsv search $sym |xsv table \
         | GREP_COLORS='mt=01;31' egrep --color=always "$sym|"
        echo
    fi
    echo -n "Search: "
done

#GREP_COLORS='mt=01;32' egrep --color=always "\[OK\]|" | GREP_COLORS='mt=01;31' egrep --color=always '\[FAIL\]|'

