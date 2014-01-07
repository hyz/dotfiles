#!/bin/sh

#tcpdump -nn -e -s 1600 -w /tmp/wood.pcap -i br-lan port 9900 or port 9000 or port 19000 or port 19900 &
#tcpdump -nn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan port 9900 or port 9000 or port 19000 &
#tcpdump -nn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan port 9900 or port 9000 or port 19000 &
#tcpdump -nn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan ether host b4:07:f9:48:3a:7b &
#tcpdump -nn -e -s 128 -w /tmp/9900.$(date +%F_%H%M) -i br-lan port 9900 &

# 9900
while true; do
    tag=`date +%H`

    tcpdump -nn -w "em1/$tag.pcap" -i em1 not net 192.168.10.0/24 and \( port 9900 or port 9000 \) &
    pid=$!

    sleep 3600
    while [ "$tag" = "`date +%H`" ]; do
	sleep 5
    done

    kill $pid ; kill -9 $pid
done

