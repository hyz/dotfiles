
https://github.com/vi/websocat

### udp echo-back

    $ websocat -t udp-l:0.0.0.0:8908 broadcast:mirror:

    > nc -u 127.0.0.1 8908

### websocket echo-back

    $ websocat -t ws-l:0.0.0.0:8908 broadcast:mirror:

    > websocat udp://127.0.0.1:8908

### 3-hop chain

    $ bin/websocat -t udp-l:0.0.0.0:8908 broadcast:mirror: # echo server

    $ bin/websocat -t tcp-l:0.0.0.0:8908 udp:206.214.70.37:8908 # middle forward

    > nc 172.16.5.4 8908 # echo client

