#
# export MY_PHONE=13520275893 MY_PASS=X${MY_PHONE} MY_HOST=127.0.0.1:8989 PEER_PHONE=13620275893

MY_DIR=`dirname $0`
. $MY_DIR/config
. $MY_DIR/functions.sh

# grep "^$MY_HOST" $MY_USER/$MY_COOKIE |awk "$3=MT{print $4}"

GET "/applyIdentify?gender=M"

