export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1

case "$2" in
    *|info)
        #moon.py GET /personInfo contactid=$1
        #moon.py GET /personResource userid={MY_UID}
        moon.py GET /personResource userid=$1
        ;;
esac

