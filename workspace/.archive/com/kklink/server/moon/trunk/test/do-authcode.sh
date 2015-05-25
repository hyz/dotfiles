export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$1" ] || exit 1

GET "/validCode?type=1&application=2&phone=$1"

