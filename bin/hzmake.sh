#!/bin/bash

# export REPO=androidbarcode VARIANT=release PLATS=k400,cvk350c,cvk350t
# hzmake.sh prepare   $REPO $VARIANT -P $PLATS
# hzmake.sh sync-up   $REPO $VARIANT -P $PLATS
# hzmake.sh rbuild    $REPO $VARIANT -P $PLATS
# hzmake.sh sync-down $REPO $VARIANT -P $PLATS
# hzmake.sh rar       $REPO $VARIANT -P $PLATS -p XXX
# hzmake.sh version-up $REPO $VARIANT -P $PLATS

die() {
    echo $* ; exit 1 ;
}
[ $# -gt 1 ] || die "$#"

builddir=$HOME/build
outdir=/samba/release1
rhost=192.168.2.113
rhome=/home/wood

what=$1 ; shift

ARGS=`getopt -o V:t:P:p:d:b: --long ver:,vertag:,plats:,rarpwd:outdir:builddir: -- $@` || exit 1
eval set -- "$ARGS" ; # echo "$@"
while true ; do
    case "$1" in
        #-v|--variant) variant=$2 ; shift 2 ;;
        -P|--plats) plats=$2 ; shift 2 ;;
        -V|--ver) NewVer=$2 ; shift 2 ;;
        -t|--vertag) Vertag="$2" ; Vertag1="-$2"; shift 2 ;;
        -p|--rarpwd) rarpwd=$2 ; shift 2 ;;
        -d|--outdir) outdir=$2 ; shift 2 ;;
        -b|--builddir) builddir=$2 ; shift 2 ;;
        --) shift ; break ;;
        *) die "opts" ; exit 1 ;;
    esac
done

repo=${1%/} #`echo "$1" | sed -e 's/^[./]\+//g' -e 's/[./]\+$//g'`
[ -d "$repo/.svn" ] || die "repo: $repo"
variant=$2
[ "$variant" = test -o "$variant" = release ] || die "variant: $variant"
[ -d "$builddir/$variant" ] || mkdir -p $builddir/$variant || die "mkdir $builddir/$variant"
#[ `stat -c\%i $builddir` = `stat -c\%i $2` ] || die "destination-dir: $2"

prjname=`basename $repo`
AppConfig=src/com/huazhen/barcode/app/AppConfig.java
#if [ "$what" = prepare -o "$what" = init ] ; then
#    svn revert $repo/$AppConfig
#fi

OldVer=`tr -d ' \t' <$repo/$AppConfig |grep -Po '^publicstaticfinalStringVERSION="v\K[^"]+'`
OldSVNRev=`tr -d ' \t' <$repo/$AppConfig |grep -Po '^publicstaticfinalStringSVNVERSION="new-svn\K[^"]+'`
if [ -z "$NewVer" ] ; then
    case "$what" in
        version-commit) NewVer=$OldVer ;;
        *) NewVer=`echo $OldVer | awk -F. '{print $1"."$2"."$3+1}'` ;;
    esac
fi
NewSVNRev=`svn info $repo |grep -Po '^Revision:\s+\K\d+'`

datestr() {
    date -d'-4HOUR' '+%Y%m%d'
}

Apk="Game-newsvn$NewSVNRev-$(datestr)$Vertag1.apk"
VerF=$Vertag$NewVer-$NewSVNRev #-$(date +%m%d) # local temp application dir
RarTag="$NewVer-`datestr`"

show-info() {
    echo "variant: $variant"
    echo "version: $OldVer => $NewVer, $OldSVNRev => $NewSVNRev, ${Apk%.apk}"
    echo
    echo "build directory: remote=$rhost:$rhome local=$builddir"
    samba='\\192.168.2.115\Release1\'
    if [ "$variant" = test ] ; then
        samba=$samba'test\'
    fi
    echo "output: samba=$samba local=$outdir"
    echo "apk: $variant/$Apk"
}

case "$what" in
info)
    show-info
    ;;

version-reset)
    svn revert $repo/$AppConfig
    ;;
version-up)
    ;;
version-commit)
    #[ "$variant" = release ] || die "commit: $variant"
    sed -i '/^\s*public.\+\<VERSION\s*=/{s/"v[0-9]\+\.[0-9]\+\.[0-9]\+"/"v'$NewVer'"/}' $repo/$AppConfig
    sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' $repo/$AppConfig
    svn diff $repo #/$AppConfig
    echo "$NewVer, $NewSVNRev, $repo/$AppConfig"
    echo "commit $repo? (y/n)"
    read yn
    case "$yn" in
        [yY]|[yY]es|YES)
            svn commit $repo -m "Version($OldVer=>$NewVer, $OldSVNRev=>$NewSVNRev) updated" ;;
    esac
    ;;

rar)
    which rar || die "rar not-found"
    for plat in ${plats//,/ } ; do
        rel="$plat-$NewVer-`datestr`"
        ar="${rel#cvk}.rar"
        rm -f $outdir/$ar
        ( cd $outdir && rar a -hp$rarpwd $ar $rel ) || die "$ar"
        echo "$outdir/$ar [OK]"
    done
    ;;

sync-down)
    for plat in ${plats//,/ } ; do
        rel="$plat-$NewVer-`datestr`"
        rsync -vrL $rhost:$variant/$rel $outdir/ || die "$rhost $rel"
    done
    ;;

rbuild)
    for plat in ${plats//,/ } ; do
        rel="$plat-$NewVer-`datestr`"
        ssh root@$rhost "cd $rhome && bin/hzrbuild.sh $variant/application/$VerF $variant/$rel $plat" \
            || echo "`date +%F\ %T` $rhost:$rhome/$variant/$rel [FAIL]" >> FAIL.`datestr`.txt # die "rbuild"
    done
    [ -r "FAIL.`datestr`.txt" ] && cat FAIL.`datestr`.txt
    ;;

sync-up)
    rm -rf /tmp/$VerF #$variant/$VerF
    mkdir -p /tmp/$VerF/lib
    mkdir -p /tmp/$VerF/internal

    cp -v $builddir/$prjname/libmtkhw.so /tmp/$VerF/lib/ || die "libmtkhw.so"
    cp -v $builddir/$prjname/libs/armeabi-v7a/libBarcode.so /tmp/$VerF/lib/ || die "libBarcode.so"
    cp -v $builddir/$variant/$Apk /tmp/$VerF/internal/Game.apk || die "$Apk"

    find /tmp/$VerF -type f -exec chmod 0644 '{}' \;
    ( cd /tmp && rsync -vr $VerF $rhost:$variant/application/ ) || die "rsync $rhost"
    rm -rf /tmp/$VerF
    ;;

ndk-build)
    ( cd $builddir/$prjname/jni && ln -s ../3rdparty . && ndk-build -j8 )
    ;;

prepare|init)
    svn st $repo

    rm -rf $builddir/$prjname
    ( cd $repo/.. && find $prjname \( -name ".svn" -o -name ".git*" \) -prune -o -print \
        | rsync --files-from=- . $builddir )

    if [ "$variant" = release ] ; then
        cp -v tools/CryptoRelease.bat $builddir/$prjname/tools/Crypto.bat
        sed -i '/^\s*public.\+\<VERSION\s*=/{s/"v[0-9]\+\.[0-9]\+\.[0-9]\+"/"v'$NewVer'"/}' $builddir/$prjname/$AppConfig
        sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' $builddir/$prjname/$AppConfig
        sed -i '/^[^#]\+#\s*define\s\+BUILD_RELEASE/{s:^[^#]\+::}' $builddir/$prjname/jni/Utils/log.h
    else
        sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' $builddir/$prjname/$AppConfig
    fi

    echo ": diff -r --brief $repo $builddir/$prjname"
    diff -r --brief $repo $builddir/$prjname |grep -v '.svn'
    echo
    diff $repo/$AppConfig $builddir/$prjname/$AppConfig
    diff $repo/jni/Utils/log.h $builddir/$prjname/jni/Utils/log.h
    #diff --brief $repo/../tools/CryptoRelease.bat $builddir/$prjname/tools/Crypto.bat

    #echo
    #find $builddir/$repo -name log.h -o -name AppConfig.java -o -name Crypto.bat

    svn log -r$OldSVNRev:$NewSVNRev $repo > svn-log.$NewVer
    echo "svn-log.$NewVer"
    show-info |tee -a svn-log.$NewVer
    ;;

esac

