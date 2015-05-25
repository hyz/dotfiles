export MY_DIR=`dirname $0`
PATH=$PATH:$MY_DIR

[ -n "$2" ] || exit 2
u1=$1
u2=$2

g=".$u1.$u2"
if [ $u1 -gt $u2 ]; then
    g=".$u2.$u1"
fi

cat |moon.py POST /sendMessage sessionid=$g type=3

