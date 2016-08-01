#!/bin/bash
# export BUILDDIR=$HOME/build RELEASEDIR=/samba/release1 RARPWD=XXX PLATS=k400,cvk350c,cvk350t
# hzmake.sh init      androidbarcode $BUILDDIR release -P $PLATS -p $RARPWD
# hzmake.sh sync-up   androidbarcode $BUILDDIR release -P $PLATS -p $RARPWD
# hzmake.sh rbuild    androidbarcode $BUILDDIR release -P $PLATS -p $RARPWD
# hzmake.sh sync-down androidbarcode $BUILDDIR release -P $PLATS -p $RARPWD
# hzmake.sh rar       androidbarcode $BUILDDIR release -P $PLATS -p $RARPWD

die() {
    echo $* ; exit 1 ;
}
[ $# -gt 1 ] || die "$#"

outdir=/samba/release1
rhost=192.168.2.113
rhome=/home/wood
builddir=$HOME/build #`echo "$2" | sed 's/[./]\+$//g'` ##/mnt/hgfs/home/svnchina/build/ #builddir=$WinTop/build

what=$1 ; shift

ARGS=`getopt -o V:t:P:p:d:b: --long ver:,vertag:,plats:,rarpwd:outdir:builddir: -- $@` || exit 1
eval set -- "$ARGS" ; # echo "$@"
while true ; do
    case "$1" in
        #-v|--variant) variant=$2 ; shift 2 ;;
        -P|--plats) plats=$2 ; shift 2 ;;
        -V|--ver) Newver=$2 ; shift 2 ;;
        -t|--vertag) Vertag="$2" ; Vertag1="-$2"; shift 2 ;;
        -p|--rarpwd) rarpwd=$2 ; shift 2 ;;
        -d|--outdir) outdir=$2 ; shift 2 ;;
        -b|--builddir) builddir=$2 ; shift 2 ;;
        --) shift ; break ;;
        *) die "opts" ; exit 1 ;;
    esac
done

repo=`echo "$1" | sed -e 's/^[./]\+//g' -e 's/[./]\+$//g'`
[ -d "$1" -a -d "$repo" ] || die "repo-dir error: $1"
variant=$2
case "$variant"  in
    test|release) ;;
    *) die "variant error: $variant" ;;
esac
[ -d "$builddir/$variant" ] || mkdir -p $builddir/$variant
#[ `stat -c\%i $builddir` = `stat -c\%i $2` ] || die "destination-dir error: $2"

AppConfig=src/com/huazhen/barcode/app/AppConfig.java
NewSVNRev=`svn info $repo |grep -Po '^Revision:\s+\K\d+'`
Ver=`tr -d ' \t' <$repo/$AppConfig |grep -Po '^publicstaticfinalStringVERSION="v\K[^"]+'`

Apk="Game-newsvn$NewSVNRev-$(date +%Y%m%d)$Vertag1.apk"
rev=$Vertag$Ver-$NewSVNRev #-$(date +%m%d) # local temp application dir

case "$what" in
clean)
    #rm -vf $builddir/$variant/$Apk
    ;;

svn-commit)
    svn diff $repo
    svn commit $repo -m "Version($Ver) updated"
    ;;

rar)
    which rar || die "rar not-found"
    for plat in ${plats//,/ } ; do
        rel="$plat-$Ver-`date +%Y%m%d`"
        ar="${rel#cvk}.rar"
        rm -f $outdir/$ar
        ( cd $outdir && rar a -hp$rarpwd $ar $rel ) || die "$ar"
        echo "$outdir/$ar [OK]"
    done
    ;;

sync-down)
    for plat in ${plats//,/ } ; do
        rel="$plat-$Ver-`date +%Y%m%d`"
        rsync -vrL $rhost:$variant/$rel $outdir/ || die "$rhost $rel"
    done
    ;;

rbuild)
    for plat in ${plats//,/ } ; do
        rel="$plat-$Ver-`date +%Y%m%d`"
        ssh root@$rhost "cd $rhome && bin/hzrbuild.sh $variant/application/$rev $variant/$rel $plat" || die "ssh rbuild"
    done

    echo "$rhost:$rhome/$variant/$rel [OK]"
    ;;

sync-up)
    rm -rf /tmp/$rev #$variant/$rev
    mkdir /tmp/$rev/lib
    mkdir /tmp/$rev/internal

    cp -v $builddir/$repo/libmtkhw.so /tmp/$rev/lib/ || die "libmtkhw.so"
    cp -v $builddir/$repo/libs/armeabi-v7a/libBarcode.so /tmp/$rev/lib/ || die "libBarcode.so"
    cp -v $builddir/$variant/$Apk /tmp/$rev/internal/Game.apk || die "$Apk"

    find /tmp/$rev -type f -exec chmod 0644 '{}' \;
    ( cd /tmp && rsync -vr $rev $rhost:$variant/application/ ) || die "rsync $rhost"
    rm -rf /tmp/$rev
    ;;

prepare|init)
    # svn up $repo
    svn revert $repo/$AppConfig
    svn st $repo
    OldSVNRev=`tr -d ' \t' <$repo/$AppConfig |grep -Po '^publicstaticfinalStringSVNVERSION="new-svn\K[^"]+'`

    if [ "$variant" = release ] ; then
        if [ -z "$Newver" ] ; then
            Newver=`echo $Ver | awk -F. '{print $1"."$2"."$3+1}'`
            [ -n "$Newver" ] || die "Incr VERSION fail"
        fi
        sed -i '/^\s*public.\+\<VERSION\s*=/{s/"v[0-9]\+\.[0-9]\+\.[0-9]\+"/"v'$Newver'"/}' $repo/$AppConfig
        sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' $repo/$AppConfig
    fi

    rm -rf $builddir/$repo
    find $repo \( -name ".svn" -o -name ".git*" \) -prune -o -print \
        | rsync --no-g --no-o --files-from=- . $builddir

    if [ "$variant" = release ] ; then
        sed -i '/^[^#]\+#\s*define\s\+BUILD_RELEASE/{s:^[^#]\+::}' $builddir/$repo/jni/Utils/log.h
        cp -v $repo/../tools/CryptoRelease.bat $builddir/$repo/tools/Crypto.bat
    else
        sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' \
            $builddir/$repo/$AppConfig
        testoutdir="\\192.168.2.115\\Release1\\test"
    fi

    echo "diff -r --brief $repo $builddir/$repo"
    diff -r --brief $repo $builddir/$repo |grep -v '.svn'
    echo
    diff $repo/$AppConfig $builddir/$repo/$AppConfig
    diff $repo/jni/Utils/log.h $builddir/$repo/jni/Utils/log.h
    diff --brief $repo/../tools/CryptoRelease.bat $builddir/$repo/tools/Crypto.bat

    #echo
    #find $builddir/$repo -name log.h -o -name AppConfig.java -o -name Crypto.bat

    echo "SVN Revision: $OldSVNRev => $NewSVNRev"
    echo "Version: $Ver => $Newver $Vertag"
    svn log -r$OldSVNRev:$NewSVNRev $repo > svn.log.$OldSVNRev-$NewSVNRev
    echo
    echo "svn.log.$OldSVNRev-$NewSVNRev"
    echo "$variant/$Apk $testoutdir"
    grep word howto.txt
    echo
    svn st $repo
    echo "svn diff $repo/$AppConfig"
    #echo "svn commit androidbarcode"
    ;;

esac

