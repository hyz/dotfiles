#
# export MY_PHONE=13520275893 MY_PASS=X${MY_PHONE} MY_HOST=127.0.0.1:8989 PEER_PHONE=13620275893

export MY_DIR=`dirname $0`
export PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1

case "$2" in
    who) moon.py GET /barFriendOnline sceneid=$1 ;;
    info) moon.py GET /barInfos sceneid=$1 ;;
    activity) moon.py GET /activeToscene sceneid=$1 ;;
    tonight) moon.py GET /activeTonight sceneid=$1 ;;
    activity-info) moon.py GET /activeInfo sceneid=$1 activeid=$3 ;;
    fav) moon.py GET /attentionsBar type=$3 sceneid=$1 ;;
    *)
        case "$1" in
            -|?|...) moon.py GET /barList city=深圳市 index=0 pageSize=20 ;;
        esac
        exit 1
        ;;
esac

exit $?

