export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

case "$1" in
    login)
        moon.py GET /login phone={MY_USER} password={MY_PASS} channel=3 type=3 systemVersion=1.0 deviceType=2 longtitude=1 latitude=1
        ;;
    regist)
        moon.py GET /regist account={MY_USER} password={MY_PASS} nickName=Nick-{MY_USER}
        ;;
    passwd)
        moon.py GET /updatePassword oldpwd=$2 newpwd=$3
        ;;
    reset-pwd-mail)
        [ -n "$2" ] || exit 2
        moon.py GET /mobileMailFindPwd type=2 mail=$2
        ;;
    bind-mail)
        [ -n "$2" ] || exit 2
        moon.py GET /bind type=2 password={MY_PASS} mail=$2
        ;;
    info)
        moon.py GET /myInfo
        ;;
    contact-list)
        moon.py GET /contactList
        ;;
    unregist)
        moon.py GET /unregister
        ;;

    hello)
        moon.py GET /hello
        ;;
    check-update)
        moon.py GET /checkUpdate deviceType=iPhone
        ;;
    help)
        moon.py GET /help
        ;;
esac

