#. $MY_DIR/config
#[ -n "$MY_USER" ] || . $MY_DIR/config
#[ -n "$MY_COOKIE" ] || MY_COOKIE=cookie

# JSV=$MY_DIR/../bin/jvalue
# [ -x "$JSV" ] || exit 9

TMP_DIR=$MY_DIR/$MY_USER
CK=$TMP_DIR/$MY_COOKIE

[ -d "$TMP_DIR" ] || mkdir -p $TMP_DIR

Presult() {
    local meth=$1
    local url=$2
    local path=$3
    local leaf=$4

    echo "$meth $url" |tee -a $TMP_DIR/${leaf}
    if ! grep 'HTTP/1.1 200' "$TMP_DIR/${leaf}"; then # if [ -z "$2" ]; then
        # errno=`$JSV "$TMP_DIR/${leaf}.G" errno`
        # if [ -n "$errno" -a "$errno" != "0" ]; then
            cat "$TMP_DIR/${leaf}.$meth"
            echo
            echo "FAIL: $path; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
            exit 1
        # fi
    # else echo "${leaf}.G $2"
    fi
    ls -l $TMP_DIR/$leaf*
}

GET() {
    #if [ -n "$MY_GWID" ]; then
    #    Extra_Args="-H mac:$MY_ETHER -H gwid:$MY_GWID"
    #fi

    local url="$1?"
    local path=`echo $url |cut -d '?' -f1`
    local qs=`echo $url |cut -d '?' -f2`
    local leaf=`basename "$path"`
    local url="http://$MY_HOST:$MY_PORT$path?$qs&PLAT=$MY_PLAT"

    echo "curl -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args $url"
    curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args "$url"  > "$TMP_DIR/${leaf}.G"
    if [ "$?" != "0" ]; then
        echo "FAIL: $MY_PLAT $?; $url; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
        exit 2
    fi
    Presult 'G' $url $path $leaf
}

POST() {
    #if [ -n "$MY_GWID" ]; then
    #    Extra_Args="-H mac:$MY_ETHER -H gwid:$MY_GWID"
    #fi

    local url="$1?"
    local path=`echo $url |cut -d '?' -f1`
    local qs=`echo $url |cut -d '?' -f2`
    local leaf=`basename "$path"`
    local url="http://$MY_HOST:$MY_PORT$path?$qs&PLAT=$MY_PLAT"

    echo "curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args $url --data-binary @-"
    curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args "$url" --data-binary "@-" > "$TMP_DIR/${leaf}.P"
    if [ "$?" != "0" ]; then
        echo "FAIL: $MY_PLAT $?; $url; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
        exit 2
    fi
    Presult 'P' $url $path $leaf
}

# curl -A $MY_PLAT -c /tmp/$USER.cj -b /tmp/$USER.cj $GP http://$MY_HOST$path --data-binary "$*"

