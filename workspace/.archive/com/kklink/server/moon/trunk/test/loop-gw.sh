#!/bin/sh
# sh ./a test.kklink.com KK1014A5

while true; do
    date
    if curl -4sS "http://$1/my-gwid?gwid=$2" -o /dev/null; then
        sleep 30
    else
        echo "fail"
        sleep 5
    fi
done

