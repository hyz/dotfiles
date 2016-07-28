#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "uid 0"
appdir=$1
outdir=$2
## cd "$appdir" || die "cd $appdir"
[ -d "$appdir" ] || die "-d $appdir"
[ -d "$outdir" ] || die "-d $outdir"

# exec >build.log 2>&1
# cat <<_EoF | /bin/sh - 2>&1 | tee build.log 

Ver=`basename $(pwd) | cut -d\- -f1`
[ -n "$Ver" ] || die "Ver: `pwd`"

Build() {
    Mt=$1
    Ot=$2 # [ -n "$Ot" ] || Ot=$Mt
    echo $Mt $Ot $Ver

    builddir="/osca/$Mt"
    [ -d "$builddir" ] || die "/osca/$Mt"

	echo "<`pwd`><$appdir><$builddir>"

    ( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
    ( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy ) || die "mk & autocopy"

    reldir0="/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`"
    reldir1="$outdir/$Mt-$Ver-`date +%Y%m%d`"

    /bin/ln -snf "$reldir0" "$reldir1" 
    #/bin/mv "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "$Mt-$Ver-`date +%Y%m%d`"
    echo $reldir1
}

# Build k400 ; exit

for m in k400 cvk350c ; do
    Build $m $m
done
Build cvk350t cvk350t-iphone

