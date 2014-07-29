#!/bin/sh
#
# @hourly cd /home/wood && sh bin/cap.sh pcap/em1/10010 -s0 -i em1 port 10010
# $ tcpdump -nn -SA -r pcap/em1/10010 host ... port ..

Tag=$1 ; shift

[ -n "$Tag" ] || exit 2
[ -d "$Tag" ] || mkdir -p $Tag || exit 3

exec >> /tmp/`basename $0`.output
exec 2>&1
echo $$
pwd

if [ -r "$Tag/pid" ] ; then
    pid=`cat $Tag/pid`
    kill $pid
    sh -c "sleep 1 ; kill -9 $pid" &
fi

echo $$ > $Tag/pid
exec /usr/sbin/tcpdump -nn -w "$Tag/`date +%H`" $*

