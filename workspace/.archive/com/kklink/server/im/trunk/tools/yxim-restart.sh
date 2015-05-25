#!/bin/sh
# /var/run/yxim-server.pid

pidfile=$1
[ -r "$pidfile" ] || exit 1

exec >>"/tmp/`echo -n "$pidfile" |tr -d '/ '`" 2>&1
date

while true ; do
    if [ -r "$pidfile" ] ; then
        pid=`cat $pidfile`

        if [ -d "/proc/$pid" ] ; then
            sleep 16
        else
            date
            /etc/init.d/yxim-server restart
        fi
    fi
    #comm=`cat /proc/$pid/comm`

    sleep 6
done

