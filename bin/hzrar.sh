#!/bin/sh

which rar || die "rar not found"
[ -n "$1" ] || die "$0 <Password> <.../build/release>"
[ -d "$2" ] || die "$0 <Password> <.../build/release>"

passwd=$1 # ; shift
cd $2

/bin/ls -1d *`date +%Y%m%d`/ |tr -d '/' |while read d ; do
    rar="${d#cvk}.rar"
    [ -e "$rar" ] && continue
    rar a -hp$passwd $rar $d
done

