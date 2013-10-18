#!/bin/sh

[ -n "$1" ] || exit 1

tb="/tmp/`basename $0`.tmp.$USER.$$.tar"

tar cf $tb $*
s=`stat -c "%s" "$tb"`

pv -s $s $tb |gzip -c - | ssh wan "gzip -dc - | tar vxf - "

rm -f "$tb"

