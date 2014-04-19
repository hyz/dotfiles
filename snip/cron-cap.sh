#!/bin/sh
# Usage: sh cap.sh pcap/em1/9000 -i em1 port 9000

Dir=$1 ; shift

[ -z "$Dir" ] && exit 1
[ -d "$Dir" ] || mkdir -p $Dir

if [ -r "$Dir/pid" ] ; then
    pid=`cat $Dir/pid`
    kill $pid && sleep 1 && kill -9 $pid
fi

echo $$ > $Dir/pid
exec tcpdump -nn -s0 -w "$Dir/`date +%H`.cap" $*

