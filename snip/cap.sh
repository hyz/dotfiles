#!/bin/sh

#tcpdump -nnnn -e -s 1600 -w /tmp/wood.pcap -i br-lan port 9900 or port 9000 or port 19000 or port 19900 &
#tcpdump -nnnn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan port 9900 or port 9000 or port 19000 &
#tcpdump -nnnn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan port 9900 or port 9000 or port 19000 &
#tcpdump -nnnn -e -s 1600 -w /tmp/$(date +%F_%H%M) -i br-lan ether host b4:07:f9:48:3a:7b &
#tcpdump -nnnn -e -s 128 -w /tmp/9900.$(date +%F_%H%M) -i br-lan port 9900 &

ifa=$1 ; shift

d=pcap.$ifa
[ -d "$d" ] || mkdir $d

while true; do
    tg="$((`date +%k` % 5))"
    tcpdump -nn -s 512 -w "$d/$tg" -i $ifa $* &
    pid=$!
    echo "$pid $d/$tg $*"
    sleep 3600
    while [ "$tg" = "$((`date +%k` % 5))" ]; do
        sleep 5
    done
    kill $pid ; kill -9 $pid
done

