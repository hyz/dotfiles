export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1
id=1

cat | moon.py POST "/sms?to=$1&id=$id"

