#!/bin/sh

which rar || die "rar not found"
[ -d "$2" ] || die "$0 <Password> <.../build/release>"
[ -n "$1" ] || die "$0 <Password> <.../build/release>"

cd $2
for d in `/bin/ls -1d */ |tr -d '/'` ; do
    rar a -hp$1 $d $d
done

