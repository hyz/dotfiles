export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1
what=$1 ; shift

case "$what" in
    new)
        moon.py POST "/createGroup" <<EoF
{ "members":[$@] }
EoF
        ;;
    *) ;;
esac

