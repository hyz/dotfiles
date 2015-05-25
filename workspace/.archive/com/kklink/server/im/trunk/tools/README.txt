
### onlmsg.py, extra_user_info.py
examples:
    $
    $ scp -P 44331 root@58.67.160.243:/home/yxim/onlmsg/1 onlmsg.txt
    $
    $ cat onlmsg.txt | ./extra_user_info.py | ./summary.py -o out-dir
    $
    $ grep 'ONL$' onlmsg.txt |./onlmsg.py |./extra_user_info.py |sort -nk3
    $
    $ grep 'MSG$' onlmsg.txt |./onlmsg.py |./extra_user_info.py |sort -nk5
    $

$ echo '{"cmd":70}' |yxim-pack |nc ims.kklink.com 10010 |python -mjson.tool

