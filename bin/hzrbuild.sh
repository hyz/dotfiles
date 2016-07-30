#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "id -u"
appdir=$1 ; out=$2 ; plat=$3 ; shift 3
## cd "$appdir" || die "cd $appdir"
[ -d "$appdir" ] || die "appdir: $appdir"
builddir="/osca/$plat"
[ -d "$builddir" ] || die "builddir: $builddir"
d=`dirname "$out"`
[ -d "$d" ] || mkdir -p "$d" || die "mkdir: $d"

# exec >build.log 2>&1
# cat <<_EoF | /bin/sh - 2>&1 | tee build.log 

#Build() {
    if [ "$plat" = "cvk350t" ] ; then
        reldir1="/osca/release/$plat-iphone-" # `date +%Y-%m-%d-%H-%M`
    else
        reldir1="/osca/release/$plat-"
    fi
    [ -d "$builddir" ] || die "$builddir"

    echo `pwd` $builddir $appdir $reldir1 $out

    ( cd $appdir && find * -type f | /bin/cpio --verbose -pu $builddir/vendor/g368_noain_t300/application ) || die "cpio"
    ( cd $builddir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy ) || die "mk & autocopy"

    #out="$out" #/$plat-$ver-`date +%Y%m%d`

    /bin/ln -snf "$reldir1`date +%Y-%m-%d-%H-%M`" "$out" 
    #/bin/mv "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "$plat-$ver-`date +%Y%m%d`"
    echo "$out [OK]"
#}
#Build $plat

# if [ $# -gt 0 ] ; then
#     while [ $# -gt 0 ] ; do
#         Build $1 ; shift 1
#     done
# else
#     # Build cvk350 ; exit
#     for x in k400 cvk350c cvk350t ; do
#         Build $x
#     done
# fi

