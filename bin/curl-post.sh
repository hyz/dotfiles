#!/bin/sh

# ./test.sh post http://127.0.0.1:9991/test_json '{"phone":13511112222,"send":true,"words":"hello world", "res":["a","b","c"]}'

get_post="--$1"
url="$2"
shift ; shift

if [ -z "$*" ]; then
    exit 1
fi
if [ "$get_post" != "--get" ]; then
    get_post=
fi

curl -A curl -c /tmp/$USER.cj -b /tmp/$USER.cj \
        $get_post $url --data-binary "$*"

# curl -A curl -c /tmp/$USER.cj -b /tmp/$USER.cj http://127.0.0.1:9990/test_json \
#     --data-binary '{"phone":13511112222,"send":true,"words":"hello world", "res":["a.wav","b.jpg","c.png"]}'

