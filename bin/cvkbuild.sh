#!/bin/sh

die() {
    echo $* ; exit 1 ;
}

ARGS=`getopt -o a:V: --long variant:,version:,upload -- $@` || exit 1
eval set -- "$ARGS" ; # echo "$@"
while true ; do
    case "$1" in
        -a|--variant) variant=$2 ; shift 2 ;;
        -V|--version) Newver=$2 ; shift 2 ;;
        --upload) _upload=1 ; shift ;;
        --) shift ; break ;;
        *) die "options" ; exit 1 ;;
    esac
done
repo=`echo "$1" | sed -e 's/^[./]\+//g' -e 's/[./]\+$//g'`
WinTop=`echo "$2" | sed 's/[./]\+$//g'` ##/mnt/hgfs/home/svnchina/
builddir=$WinTop/build

[ -d "$repo" -a -d "$1" ] || die "repo-dir error: $1"
[ -d "$builddir" -a -d "$2" ] || die "destination-dir error: $2"
case "$variant"  in
    test) ;;
    release) ;;
    *) die "variant error: $variant" ;;
esac

svn st $repo

AppConfig=$repo/src/com/huazhen/barcode/app/AppConfig.java

NewSVNRev=`svn info $repo |grep -Po '^Revision:\s+\K\d+'`
Ver=`tr -d ' \t' <$AppConfig |grep -Po '^publicstaticfinalStringVERSION="v\K[^"]+'`
Apk="Game-newsvn$NewSVNRev-$(date +%Y%m%d).apk"

if [ -n "$_upload" ] ; then
    [ "$variant" = release ] || die "release required"

    host_ip=192.168.2.113
    #cwd=`pwd`
    #vendor/g368_noain_t300/application/lib/libmtkhw.so
    #vendor/g368_noain_t300/application/internal/Game.apk
    reldir0=build/$variant/$Ver-$NewSVNRev-$(date +%m%d%H%M)
    reldir=$reldir0/application
    [ -d "$reldir/lib" ] || mkdir -p $reldir/lib || die "$reldir/lib"
    [ -d "$reldir/internal" ] || mkdir -p $reldir/internal || die "$reldir/internal"

    cp -v $builddir/$repo/libmtkhw.so $reldir/lib/ || die "libmtkhw.so"
    cp -v $builddir/$repo/libs/armeabi-v7a/libBarcode.so $reldir/lib/ || die "libBarcode.so"
    cp -v $builddir/$variant/$Apk $reldir/internal/Game.apk || die "$Apk"

    find $reldir -type f -exec chmod a-x '{}' \;
    rsync -vrR $reldir0 $host_ip:. || die "$host_ip"

    ssh root@$host_ip "/bin/bash /home/wood/bin/mk_r.sh /home/wood/$reldir0" || die "ssh mk_r.sh"

    rels=`ssh $host_ip "/bin/ls -1d $reldir0/out/*"`
    for r in $rels ; do
        rsync -vrL $host_ip:$r $builddir/$variant/ || die "$host_ip:$r"
    done

    echo "mk-rar.sh <Password> $builddir/$variant"

    echo "Copyed: $host_ip $reldir0"
    exit
fi
if [ -n "$_download" ] ; then
    exit
fi

if [ "$variant" = release ] ; then
    if [ -z "$Newver" ] ; then
        Newver=`echo $Ver | awk -F. '{print $1"."$2"."$3+1}'`
        [ -n "$Newver" ] || die "Incr VERSION fail"
    fi
    sed -i '/^\s*public.\+\<VERSION\s*=/{s/"v[0-9]\+\.[0-9]\+\.[0-9]\+"/"v'$Newver'"/}' $AppConfig
    sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' $AppConfig
fi

rm -rf $builddir/$repo
find $repo \( -name ".svn" -o -name ".git*" \) -prune -o -print \
    | rsync --no-g --no-o --files-from=- . $builddir

if [ "$variant" = release ] ; then
    sed -i '/^[^#]\+#\s*define\s\+BUILD_RELEASE/{s:^[^#]\+::}' $builddir/$repo/jni/Utils/log.h
    cp -v variant/release/CryptoRelease.bat $builddir/$repo/tools/Crypto.bat
else
    sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' \
        $builddir/$AppConfig
fi
# jni/Utils/log.h
# src/com/huazhen/barcode/app/AppConfig.java
#   public static final String VERSION ="v1.3.35";
#   public static final String SVNVERSION ="new-svn637";
# jni/Render/VideoRender.cpp

diff -r --brief $repo $builddir/$repo |grep -v '.svn'
echo
diff $AppConfig $builddir/$AppConfig
diff $repo/jni/Utils/log.h $builddir/$repo/jni/Utils/log.h
diff --brief variant/release/CryptoRelease.bat $builddir/$repo/tools/Crypto.bat

echo
find $builddir/$repo -name log.h -o -name AppConfig.java -o -name Crypto.bat
svn st $repo

echo "diff -r --brief $repo $builddir/$repo"
echo "svn diff $AppConfig"
echo
OldSVNRev=`tr -d ' \t' <$AppConfig |grep -Po '^publicstaticfinalStringSVNVERSION="new-svn\K[^"]+'`
echo "Revision: $OldSVNRev => $NewSVNRev"
echo "Version: $Ver => $Newver"
echo "Output required: $variant/$Apk"

###
# ls $builddir/$Apk
# find $builddir/$repo -name libmtkhw.so -o -name libBarcode.so

