#!/bin/bash
## gz.sh . ..
## gz.sh . /tmp
## gz.sh . - |ssh rhost "cat > $(date +%m%d.%H).cpio.gz"
## gzip -dc $zdir.cpio.gz-`date +%F` |cpio -idu

[ $# = 2 ] || exit 2

if [ "$2" = - ] ; then
    oar="$2"
else
    outdir="`cd $2 && pwd`" || exit 1
fi

cd $1 || exit 1

zdir=`basename $PWD`
[ -n "$oar" ] || oar="$outdir/$zdir.cpio.gz-`date +%m%d.%H`"

cd ..
##echo $PWD $zdir $oar ; exit

if [ "$oar" != - ] ; then
    exec 3>&1 >$oar
fi

find -P $zdir -type f \( \
    -name "*.h" -o -name "*.hpp" \
    -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
    -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
  \) |cpio -o |gzip -c # >$oar

if [ "$oar" != - ] ; then
    exec 1>&3
    ls -hl $oar
fi

