export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

what=$1 ; shift

case "$what" in
    throw)
        type=${1:-4}
        fileName=${2:-bottle}.$type
        moon.py POST "/sendBottle?type=$type&fileName=$fileName&deviceType=PY"
        ;;
    rethrow)
        [ -n "$1" ] || exit 1
        moon.py GET "/throwBottle?bottleid=$1&deviceType=PY"
        ;;
    catch)
        moon.py GET "/receiveBottle?deviceType=PY"
        ;;
    *) moon.py GET "/myBottle" ;;
esac

