#!/bin/bash

die() {
    xcode=$?
    echo $* ; exit $xcode ;
}

DEST=${DEST:-.}

ARGS=`getopt -o d:V --long target-directory:,help,version -- $@` || exit 1
eval set -- "$ARGS" # ; echo "$@"
while true ; do
    case "$1" in
        -d|--target-directory) DEST="${2%/}" ; shift 2 ;;
        -s|--suffix) SUFFIX="${2%/}" ; shift 2 ;;
        -V|--version) Version ; exit ;;
        --help) Help ; exit ;;
        --) shift ; break ;;
        *) die "$@" ; exit 1 ;;
    esac
done

[ $# -gt 0 ] || die "$*"

echo -e "DIR: $DEST \t--: $*" >&2
[ -d "$DEST" ] || die "$DEST"

AF="`date +%y%m%d`"
SUFFIX=`basename "$1"`

mkdir $DEST/$AF || die "mkdir $DEST/$AF"
trap '/bin/rm -rf $DEST/$AF' INT TERM EXIT

for x in $* ; do
    rsync -a ${x%/} $DEST/$AF/ || die "$x"
done

( cd $DEST && tar czf "$AF.$SUFFIX.tgz" $AF ) && /bin/rm -rf $DEST/$AF || die "$AF.tgz"
ls -l $DEST/$AF.$SUFFIX.tgz

