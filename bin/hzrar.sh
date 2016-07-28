#!/bin/sh

die() {
    echo $* ; exit 1 ;
}

which rar || die "rar not found"
[ -n "$1" ] || die "$0 <Password> <.../build/release>"
[ -d "$2" ] || die "$0 <Password> <.../build/release>"

passwd=$1 # ; shift
cd $2

/bin/ls -1d *`date +%Y%m`??/ |tr -d '/' |while read d ; do
    ar="${d#cvk}.rar"
    if [ -e "$ar" ] ; then
        echo "Already exist: $ar"
        continue
    fi
    rar a -hp$passwd $ar $d
done

