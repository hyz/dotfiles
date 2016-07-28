#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "id -u"
appdir=$1 ; outdir=$2 ; ver=$3 ; shift 3
## cd "$appdir" || die "cd $appdir"
[ -d "$appdir" ] || die "appdir: $appdir"
[ -d "$outdir" ] || die "outdir: $outdir"
[ -n "$ver" ] || die "ver"

# exec >build.log 2>&1
# cat <<_EoF | /bin/sh - 2>&1 | tee build.log 

Build() {
    Mt=$1 ; Ot=$1
    [ "$Mt" = "cvk350t" ] && Ot="$Mt-iphone"
    echo $Mt $Ot $ver

    builddir="/osca/$Mt"
    [ -d "$builddir" ] || die "/osca/$Mt"

	echo "<`pwd`><$appdir><$builddir> ==="

    ( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
    ( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy ) || die "mk & autocopy"

    reldir0="/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`"
    reldir1="$outdir/$Mt-$ver-`date +%Y%m%d`"

    /bin/ln -snf "$reldir0" "$reldir1" 
    #/bin/mv "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "$Mt-$ver-`date +%Y%m%d`"
    echo "$reldir1 [OK]"
}

if [ $# -gt 0 ] ; then
    while [ $# -gt 0 ] ; do
        Build $1 ; shift 1
    done
else
    # Build cvk350 ; exit
    for x in k400 cvk350c cvk350t ; do
        Build $x
    done
fi

