#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

DEST=${DEST:-.}

ARGS=`getopt -o t:V --long target-directory:,help,version -- $@` || exit 1
eval set -- "$ARGS" # ; echo "$@"
while true ; do
    case "$1" in
        -t|--target-directory) DEST="${2%/}" ; shift 2 ;;
        -V|--version) Version ; exit ;;
        --help) Help ; exit ;;
        --) shift ; break ;;
        *) die "$@" ; exit 1 ;;
    esac
done

echo -e "DIR: $DEST \t--: $*" >&2
[ -d "$DEST" ] || die "$DEST"

AF="`date +%y%m%d`"
mkdir $DEST/$AF || die "mkdir $DEST/$AF"
trap '/bin/rm -rf $DEST/$AF' INT TERM EXIT

for x in $* ; do
    rsync -a ${x%/} $DEST/$AF/ || die "$x"
done

( cd $DEST ; tar czf "$AF.tgz" $AF ) && rm -rf $DEST/$AF || die "$AF.tgz"
ls -l $DEST/$AF.tgz

