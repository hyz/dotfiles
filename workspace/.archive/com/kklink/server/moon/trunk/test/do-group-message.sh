export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1

case "$2" in
    image)
        [ -f "$3" ] || exit 3
        cat "$3" | moon.py POST /sendMessage sessionid=$1 type=1 filename="`basename $3`"
        ;;
    global)
        cat | moon.py POST /publicNotify type=3
        ;;
    *|text)
        cat | moon.py POST /sendMessage sessionid=$1 type=3
        ;;
esac

