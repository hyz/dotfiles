#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "id -u"

appdir=$1
out=$2
plat=$3
builddir="/osca/$plat"

[ -d "$appdir" ] || die "appdir: $appdir"
[ -d "$builddir" ] || die "builddir: $builddir"

dn=`dirname "$out"`
[ -d "$dn" ] || mkdir -p "$dn" || die "mkdir: $dn"

if [ "$plat" = "cvk350t" ] ; then
    reldir1="/osca/release/$plat-iphone-" # `date +%Y-%m-%d-%H-%M`
else
    reldir1="/osca/release/$plat-"
fi

echo `pwd` $builddir $appdir $reldir1 $out

( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy ) || die "mk & autocopy"

/bin/ln -snf "$reldir1`date +%Y-%m-%d-%H-%M`" "$out" 
#/bin/mv "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "$plat-$ver-`date +%Y%m%d`"
echo "$out [OK]"

