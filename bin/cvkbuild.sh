#!/bin/sh

err_exit() {
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
        *) err_exit "options" ; exit 1 ;;
    esac
done
repo=`echo "$1" | sed -e 's/^[./]\+//g' -e 's/[./]\+$//g'`
builddir=`echo "$2" | sed 's/[./]\+$//g'` ##/mnt/hgfs/home/svnchina/
cwd=`pwd`

[ -d "$repo" -a -d "$1" ] || err_exit "repo-dir error: $1"
[ -d "$builddir" -a -d "$2" ] || err_exit "destination-dir error: $2"
case "$variant"  in
    test|release) ;;
    *) err_exit "variant error: $variant" ;;
esac

svn st $repo

NewSVNRev=`svn info $repo |grep -Po '^Revision:\s+\K\d+'`
Apk="Game-newsvn$NewSVNRev-$(date +%Y%m%d).apk"

if [ x"$_upload" = x1 ] ; then
    # echo "cd \"$builddir\" && rsync -vr build-apk $cwd" |sh -
    reldir=build-release/$(date +%F)
    mkdir -p $reldir
    cp -v $builddir/$repo/libmtkhw.so $builddir/$repo/libs/armeabi-v7a/libBarcode.so $reldir/
    cp -v $builddir/build-apk/$Apk $reldir/Game.apk
    find $reldir -type f -exec chmod a-x '{}' \;
    rsync -vr $reldir 192.168.2.113:build-release/
    exit
fi

AppConfig=$repo/src/com/huazhen/barcode/app/AppConfig.java

OldSVNRev=`tr -d ' \t' <$AppConfig |grep -Po '^publicstaticfinalStringSVNVERSION="new-svn\K[^"]+'`
Oldver=`tr -d ' \t' <$AppConfig |grep -Po '^publicstaticfinalStringVERSION="v\K[^"]+'`

if [ "$variant" = release ] ; then
    if [ -z "$Newver" ] ; then
        Newver=`echo $Oldver | awk -F. '{print $1"."$2"."$3+1}'`
        [ -n "$Newver" ] || err_exit "Incr VERSION fail"
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

echo
echo "variant: $variant"
echo "Revision: $OldSVNRev => $NewSVNRev"
echo "Version: $Oldver => $Newver"
echo "diff -r --brief $repo $builddir/$repo"
echo "svn diff $AppConfig"

echo "$Apk"
###
# ls $builddir/build-apk/Game-newsvn$NewSVNRev-$(date +%Y%m%d).apk
# find $builddir/$repo -name libmtkhw.so -o -name libBarcode.so

