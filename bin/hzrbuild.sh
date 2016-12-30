#!/bin/bash

die() {
    echo >&2 $* ; exit 127 ;
}

[ "`id -u`" = 0 ] || die "id -u"

# appdir=$1 ; out=$2 ; plat=$3 ; shift 3
appdir=$1
out=$2
plat=$3
builddir="/osca/$plat"

[ -n "`basename $out`" ] || die "'basename $out'"
[ -d "$appdir" ] || die "appdir: $appdir"
[ -d "$builddir" ] || die "builddir: $builddir"
dn=`dirname "$out"`
[ -d "$dn" ] || mkdir -p "$dn" || die "mkdir: $dn"

echo `pwd` $builddir $appdir $out

( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage ) || die "mk"
# mtkbuild-copyout.sh $plat $out || die "copy"

out1="/osca/release/`basename $out`"
[ ! -e "$out1" ] || rm -rf "$out1"
mkdir -p $out1

project=g368_nyx
srcprj=out/target/product/$project
( cd /osca/$plat && /bin/bash copyResult "$out1" $srcprj $project ) || die "copy"
ln -vsnf "$out1" "$out" 
echo "[OK]"

