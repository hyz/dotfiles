#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "id -u"
appdir=$1 ; outdir=$2 ; shift 2
## cd "$appdir" || die "cd $appdir"
[ -d "$appdir" ] || die "-d $appdir"
[ -d "$outdir" ] || die "-d $outdir"

# exec >build.log 2>&1
# cat <<_EoF | /bin/sh - 2>&1 | tee build.log 

ver=`basename $(pwd) | cut -d\- -f1`
[ -n "$ver" ] || die "ver: `pwd`"

Build() {
    Mt=$1
    Ot=$2 # [ -n "$Ot" ] || Ot=$Mt
    echo $Mt $Ot $ver

    builddir="/osca/$Mt"
    [ -d "$builddir" ] || die "/osca/$Mt"

	echo "<`pwd`><$appdir><$builddir>"

    ( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
    ( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy ) || die "mk & autocopy"

    reldir0="/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`"
    reldir1="$outdir/$Mt-$ver-`date +%Y%m%d`"

    /bin/ln -snf "$reldir0" "$reldir1" 
    #/bin/mv "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "$Mt-$ver-`date +%Y%m%d`"
    echo $reldir1
}

if [ $# -gt 1 ] ; then
    while [ $# -gt 1 ] ; do
        x=$1 ; y=$2 ; shift 2
        Build $x $y
    done
else
    # Build cvk350 ; exit
    for x in k400 cvk350c ; do
        Build $x $x
    done
    Build cvk350t cvk350t-iphone
fi

