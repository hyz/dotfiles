
https://github.com/vi/websocat

### udp echo-back

    $ websocat -t udp-l:0.0.0.0:9012 broadcast:mirror:

    > nc -u 127.0.0.1 9012

### websocket echo-back

    $ websocat -t ws-l:0.0.0.0:9012 broadcast:mirror:

    > websocat udp://127.0.0.1:9012

