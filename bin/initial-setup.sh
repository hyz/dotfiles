#!/bin/sudo /bin/bash
## $XHOME Downloads Music Movies Pictures Documents Desktop

mount /xhome

#zsh --login - <<< 'cargo mk ftp-mount'
cd /home/ftp
echo "$SHELL `pwd` `id` $LOGNAME "
for x in `/bin/find ???* -prune -type d` ; do
    src=`/bin/find ../library/* -prune -type d -name "$x*"`
    echo "mount -o bind $src $x"
    [ -d "$x" -a -d "$src" ] || continue
    mount -o bind "$src" "$x"
done

shutdown -c ; shutdown -h 00:10
bftpd -d

exit

die() {
    xcode=$?
    echo >&2 $*
    exit $xcode
}
if [[ "`id -ru`" = 0 ]] ; then
    finalize() {
        [ $? -eq 0 ] || umount $XHOME
    }
    trap finalize INT TERM EXIT

    echo >&2 -e "$* \t: $(id) \tSUDO_USER=$SUDO_USER \t: $(pwd) "

    for xd in $* ; do
        xs=`basename $xd`
        echo >&2 "bind: $XHOME/$xs $xd"
        mount -o bind $XHOME/$xs $xd || exit # die "$XHOME/$xs $xd"
    done

    exit
fi

echo "XHOME=$XHOME"
[ -d "$XHOME" ] || die "'$XHOME'"

DEST=${DEST:-.}
ARGS=`getopt -o t:V --long target-directory:,help,version -- $@` || exit 1

eval set -- "$ARGS" # ; echo "$@"
while true ; do
    case "$1" in
        -t|--target-directory) DEST="${2%/}" ; shift 2 ;;
        -V|--version) Version ; exit ;;
        --help) Help ; exit ;;
        --) shift ; break ;;
        *) die "$@" ; exit 1 ;;
    esac
done
[ -d "$DEST" ] || die "$DEST"
#echo >&2 -e "DIR: $DEST \t--: $*"

sudo mount $XHOME || exit
trap 'sudo umount $XHOME' INT TERM EXIT

for xd in $* ; do
    xs=`basename $xd`
    [ -d "$XHOME/$xs" ] || die $BASH_LINENO "$XHOME/$xs"
    [ -d "$xd" ] || mkdir $xd || die $BASH_LINENO "$xd"
done

exec sudo DEST=$DEST /bin/bash $0 $*

#/bin/cat << @EoF | sudo /bin/bash -
#    Should not reachable
#@EoF

