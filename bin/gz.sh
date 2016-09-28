#!/bin/bash
## gz.sh . ..
## gz.sh . /tmp
## gzip -dc $zdir.cpio.gz-`date +%F` |cpio -idu

[ $# = 2 ] || exit 2

outdir="`cd $2 && pwd`" || exit 1

cd $1 || exit 1

zdir=`basename $PWD`
oar="$outdir/$zdir.cpio.gz-`date +%m%d.%H`"

cd ..
##echo $PWD $zdir $oar ; exit

find -P $zdir -type f \( \
    -name "*.h" -o -name "*.hpp" \
    -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
    -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
  \) |cpio -o |gzip -c > $oar

ls -hl $oar

