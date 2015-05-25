#
# export MY_PHONE=13520275893 MY_PASS=X${MY_PHONE} MY_HOST=127.0.0.1:8989 PEER_PHONE=13620275893

MY_DIR=`dirname $0`
. $MY_DIR/config
. $MY_DIR/functions.sh

[ -n "$2" ] || exit 2

# GET "/ap?message=$(date +%F)&devtok=$1&id=1234"

if [ -n "$3" ]; then
    echo -n "$3" | POST "/ap?devtok=$1&id=$2"
    exit $?
fi

cat | POST "/ap?devtok=$1&id=$2"

