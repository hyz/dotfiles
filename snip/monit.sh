#!/bin/sh

arg0=$1
pidfile=$2
shift ; shift

case "$arg0" in
    start)
        cmds=$*
        [ -n "$cmds" ] || exit 1
        echo $$ > $pidfile
        exec $cmds
        ;;
    stop)
        kill `cat $pidfile`
        sleep 1
        kill -9 `cat $pidfile`
        ;;
    *)  echo "usage: [start|stop] <pidfile> <command...>" ;;
esac
exit 0

# check process xyz with pidfile /var/run/xyz.pid
#        start = "/bin/xyz start /var/run/xyz.pid /usr/local/bin/moon -c"
#        stop = "/bin/xyz stop"

