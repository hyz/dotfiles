#!/bin/bash

# export REPO=androidbarcode VARIANT=release PLATS=k400,cvk350c,cvk350t
# hzmake.sh prepare    $REPO $VARIANT -P $PLATS
# hzmake.sh sync-up    $REPO $VARIANT -P $PLATS
# hzmake.sh rbuild     $REPO $VARIANT -P $PLATS
# hzmake.sh sync-down  $REPO $VARIANT -P $PLATS
# hzmake.sh rar        $REPO $VARIANT -P $PLATS -p XXX
# hzmake.sh svn-commit $REPO $VARIANT -P $PLATS

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
        -V|--ver) Newver=$2 ; shift 2 ;;
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

AppConfig=src/com/huazhen/barcode/app/AppConfig.java
if [ "$what" = prepare -o "$what" = init ] ; then
    svn revert $repo/$AppConfig
fi
NewSVNRev=`svn info $repo |grep -Po '^Revision:\s+\K\d+'`
Ver=`tr -d ' \t' <$repo/$AppConfig |grep -Po '^publicstaticfinalStringVERSION="v\K[^"]+'`

Apk="Game-newsvn$NewSVNRev-$(date +%Y%m%d)$Vertag1.apk"
Rev=$Vertag$Ver-$NewSVNRev #-$(date +%m%d) # local temp application dir

case "$what" in
clean)
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
        ssh root@$rhost "cd $rhome && bin/hzrbuild.sh $variant/application/$Rev $variant/$rel $plat" || die "ssh rbuild"
    done

    echo "$rhost:$rhome/$variant/$rel [OK]"
    ;;

sync-up)
    rm -rf /tmp/$Rev #$variant/$Rev
    mkdir -p /tmp/$Rev/lib
    mkdir -p /tmp/$Rev/internal

    svname=`basename $repo`
    cp -v $builddir/$svname/libmtkhw.so /tmp/$Rev/lib/ || die "libmtkhw.so"
    cp -v $builddir/$svname/libs/armeabi-v7a/libBarcode.so /tmp/$Rev/lib/ || die "libBarcode.so"
    cp -v $builddir/$variant/$Apk /tmp/$Rev/internal/Game.apk || die "$Apk"

    find /tmp/$Rev -type f -exec chmod 0644 '{}' \;
    ( cd /tmp && rsync -vr $Rev $rhost:$variant/application/ ) || die "rsync $rhost"
    rm -rf /tmp/$Rev
    ;;

prepare|init)
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

    svname=`basename $repo`
    rm -rf $builddir/$svname
    ( cd $repo/.. && find $svname \( -name ".svn" -o -name ".git*" \) -prune -o -print \
        | rsync --files-from=- . $builddir )

    if [ "$variant" = release ] ; then
        sed -i '/^[^#]\+#\s*define\s\+BUILD_RELEASE/{s:^[^#]\+::}' $builddir/$svname/jni/Utils/log.h
        cp -v tools/CryptoRelease.bat $builddir/$svname/tools/Crypto.bat
    else
        sed -i '/^\s*public.\+\<SVNVERSION\s*=/{s/"new-svn[0-9]\+"/"new-svn'$NewSVNRev'"/}' \
            $builddir/$svname/$AppConfig
        testoutdir=" \\\\192.168.2.115\\Release1\\test\\"
    fi

    echo ": diff -r --brief $repo $builddir/$svname"
    diff -r --brief $repo $builddir/$svname |grep -v '.svn'
    echo
    diff $repo/$AppConfig $builddir/$svname/$AppConfig
    diff $repo/jni/Utils/log.h $builddir/$svname/jni/Utils/log.h
    #diff --brief $repo/../tools/CryptoRelease.bat $builddir/$svname/tools/Crypto.bat

    #echo
    #find $builddir/$repo -name log.h -o -name AppConfig.java -o -name Crypto.bat

    echo "SVN Revision: $OldSVNRev => $NewSVNRev"
    echo "Version: $Ver => $Newver $Vertag"
    echo
    svn log -r$OldSVNRev:$NewSVNRev $repo > svn.log.$OldSVNRev-$NewSVNRev
    echo "svn.log.$OldSVNRev-$NewSVNRev"
    echo "$variant/$Apk$testoutdir"
    # grep word howto.txt
    echo
    svn st $repo
    #echo "svn diff $repo/$AppConfig"
    #echo "svn commit androidbarcode"
    ;;

esac

