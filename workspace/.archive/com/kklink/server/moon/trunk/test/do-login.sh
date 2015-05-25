#
# export MY_PHONE=13520275893 MY_PASS=X${MY_PHONE} MY_HOST=127.0.0.1:8989 PEER_PHONE=13620275893

# MY_DIR=`dirname $0`
# . $MY_DIR/config
# . $MY_DIR/functions.sh

#unset MY_GWID

#GET "/login?channel=3&macAddress=$MY_ETHER&type=3&phone=$MY_USER&password=$MY_PASS&systemVersion=1.0&deviceType=2&longtitude=1&latitude=1"

export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

#moon.py LOGIN /login channel=3 type=3 systemVersion=1.0 deviceType=2 longtitude=1 latitude=1
moon.py GET /login \
    phone={MY_USER} password={MY_PASS} channel=3 type=3 systemVersion=1.0 deviceType=2 longtitude=1 latitude=1

