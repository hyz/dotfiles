#!/bin/sh

#for x in $*; do
#    iconv -f gbk -t utf-8 "$x"
#done

filename="$1"
if [ ! -e "$filename" ] ; then
    echo >&2 "$filename"
    exit 1
fi

if [ -f "$filename.orig.gbk" ] ; then
    echo "exist: $filename.orig.gbk"
    exit 0
fi

if file "$filename" | grep "UTF-8" ; then
    exit 0
fi
if iconv -f gbk -t utf-8 "$filename" > "$filename.utf8" ; then
    mv "$filename" "$filename.orig.gbk"
    mv "$filename.utf8" "$filename"
    which dos2unix >/dev/null && dos2unix "$filename"
    exit 0
fi

