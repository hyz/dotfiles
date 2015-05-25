export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1

case "$2" in
    remove)
        [ -n "$3" ] || exit 3
        moon.py GET /delGroupMember sessionid=$1 memberid=$3
        ;;
    invite)
        [ -n "$3" ] || exit 3
        echo "{\"members\":[$3]}" |moon.py POST /inviteJoinGroup sessionid=$1
        ;;
    join)
        moon.py GET /joinGroup sessionid=$1
        ;;
    *)
        moon.py GET /memberList sessionid=$1
        ;;
esac

