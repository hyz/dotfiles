#!/bin/sh

# export MY_PHONE=13511112222
# export MY_PASS=xxxyyy
# export MY_HOST=192.168.1.55:8888
#
# ./test.sh post http://127.0.0.1:9991/test_json '{"phone":13511112222,"send":true,"words":"hello world", "res":["a","b","c"]}'

[ -n "$MY_PHONE" ] || MY_PHONE=13511112222
[ -n "$MY_PASS" ] || MY_PASS=13511112222
[ -n "$MY_HOST" ] || MY_HOST=192.168.1.55:8899

GP="--$1"
path="$2"
shift ; shift

if [ -z "$path" ]; then
    exit 1
fi
if [ "$GP" != "--get" ]; then
    GP=
fi

curl -A cURL -c /tmp/$USER.cj -b /tmp/$USER.cj http://$MY_HOST/logout
curl -A cURL -c /tmp/$USER.cj -b /tmp/$USER.cj "http://$MY_HOST/login?phone=$MY_PHONE&password=$MY_PASS&fr=cURL"
curl -A cURL -c /tmp/$USER.cj -b /tmp/$USER.cj $GP http://$MY_HOST$path --data-binary "$*"

# curl -A cURL -c /tmp/$USER.cj -b /tmp/$USER.cj http://127.0.0.1:9990/test_json \
#     --data-binary '{"phone":13511112222,"send":true,"words":"hello world", "res":["a.wav","b.jpg","c.png"]}'

# http://192.168.1.55:8989/register?phone=13511112222&password=13511112222&name=woodoo&sex=1&code=343287&fr=cURL

