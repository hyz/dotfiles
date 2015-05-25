#
# export MY_PHONE=13520275893 MY_PASS=X${MY_PHONE} MY_HOST=127.0.0.1:8989 PEER_PHONE=13620275893

MY_DIR=`dirname $0`
. $MY_DIR/config
. $MY_DIR/functions.sh

# [ -n "$2" ] || exit 2

cat | POST "/bills?channel=3&pid=8&environment=+'Sandbox'"

