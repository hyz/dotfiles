#!/bin/bash
## gz.sh . ..
## gz.sh . /tmp
## gz.sh . - |ssh rhost "cat > $(date +%m%d.%H).cpio.gz"
## gzip -dc ...cpio.gz |cpio -t
## gzip -dc ...cpio.gz |cpio -idu

[ $# = 2 ] || exit 2

if [ -z "$2" -o "$2" = - ] ; then
    out=
else
    out="`cd $2 && pwd`" || exit 3
fi

cd $1 || exit 4

src=`basename $PWD`
[ -z "$out" ] || out="$out/$src.cpio.gz-`date +%m%d.%H`"

cd ..
##echo $PWD $src $out ; exit

if [ -n "$out" ] ; then
    exec 3>&1 >$out
fi

find -P $src \( -name node_modules -o -name target -o -name dist -o -iname build \) -prune -o -type f -print \
   |cpio -o |gzip -c

#find -P $src -type f \( \
#    -name "*.h" -o -name "*.c" \
#    -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
#    -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
#    -o -name "*.pro" -o -name "*.qrc" \
#  \) |cpio -o |gzip -c

if [ -n "$out" ] ; then
    exec 1>&3 3>&-
    ls -hl $out
fi

