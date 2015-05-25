
sid=_2106
export MY_USER=test-3
export MY_HOST=moon.kklink.com
export MY_DIR=`dirname $0`

function login {
    . $MY_DIR/config
    . $MY_DIR/functions.sh
    sh do-login.sh
}

# GET "/regist?account=$MY_USER&password=$MY_PASS&nickName=Nick-$MY_USER"
# for i in {1..25} ; do echo -n test-$i " "; perl -wlne 'print $1 if /"userid":(\d+)/' test-$i/regist.G ;done

function sendx {
    . $MY_DIR/config
    . $MY_DIR/functions.sh
    date | POST "/sendMessage?sessionid=$1&type=3"
}

login
while true; do
    date
    sendx $sid
    date
    sleep 3
done

