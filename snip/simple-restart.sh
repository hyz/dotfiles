#!/bin/sh
# /var/run/yxim-server.pid

pidfile=$1
[ -n "$pidfile" ] || exit 1

#prog=$2
#[ -n "$prog" ] || exit 2

exec >/tmp/yxim.simple-restart 2>&1

while true ; do
    if [ -r "$pidfile" ] ; then
        pid=`cat $pidfile`

        if [ -d "/proc/$pid" ] ; then
            sleep 5
        else
            /etc/init.d/yxim-server restart
        fi
    fi

    #comm=`cat /proc/$pid/comm`

    sleep 3
done

