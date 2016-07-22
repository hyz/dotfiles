#!/bin/bash

die() {
    echo $* ; exit 1 ;
}

[ "`id -u`" = 0 ] || die "uid 0"
[ -d "$1" ] || die "apk dir $1"
cd "$1"

# exec >build.log 2>&1
# cat <<_EoF | /bin/sh - 2>&1 | tee build.log 

Ver=`basename $(pwd) | cut -d\- -f1`
[ -n "$Ver" ] || die "Ver error: `pwd`"

Build() {
    Mt=$1
    Ot=$2
    [ -n "$Ot" ] || Ot=$Mt
    echo $Mt $Ot $Ver

    Bdir="/osca/$Mt"
    [ -d "$Bdir" ] || die "/osca/$Mt"

	echo "`pwd` $Bdir"

	find application -type f |cpio --verbose -pu $Bdir/vendor/g368_noain_t300 || die "cpio"
	/bin/bash -c "cd $Bdir && ./mk -o=TARGET_BUILD_VARIANT=user systemimage && ./autocopy" || die "mk & autocopy"

    ln -snf "/osca/release/$Ot-`date +%Y-%m-%d-%H-%M`" "out/$Mt-$Ver-`date +%Y%m%d`"
    echo "out/$Mt-$Ver-`date +%Y%m%d`"
}

[ -d out ] || mkdir out

Build k400 ; exit

for m in cvk350c k400 ; do
    Build $m
done
Build cvk350t cvk350t-iphone

