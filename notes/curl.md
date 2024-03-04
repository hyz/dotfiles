
    curl -C - -LO http://...

    curl --socks5 192.168..:1080 --proxy-anyauth -I https://google.com

### ...

Test direct proxying with cURL:

    curl -kvx localhost:8080 http://www.google.com/humans.txt
    curl -kvx localhost:8080 https://www.google.com/humans.txt

Test HTTP connect with cURL:

    curl -kpvx localhost:8080 http://www.google.com/humans.txt
    curl -kpvx localhost:8080 https://www.google.com/humans.txt

json

    curl -w'\n' -H "Content-Type: application/json" -XPOST -d '{"key": "value"}' http:localhost:3088/post

### http://curl.haxx.se/

### http://stackoverflow.com/questions/27611193/curl-ssl-with-self-signed-certificate

    curl -k url
    curl --insecure url

### https://serverfault.com/questions/199434/how-do-i-make-curl-use-keepalive-from-the-command-line

    curl -v http://my.server/url1 http://my.server/url2

### https://stackoverflow.com/questions/6086609/how-to-do-keepalive-http-request-with-curl
### https://curl.haxx.se/libcurl/c/persistant.html

re-using handles to do HTTP persistent connections

Maybe *memory-leak* will happend in this mode..

### https://stackoverflow.com/questions/972925/persistent-keepalive-http-with-the-php-curl-library

###

    curl --connect-timeout 2 -x 127.0.0.1:8118 http://google.com


    curl -x "http://127.0.0.1:3128" "http://httpbin.org/ip" 
    curl -x "socks5://127.0.0.1:1080" "https://www.google.com"

    LD_PRELOAD=libtsocks.so w3m ...


export ec=18; while [ $ec -eq 18 ]; do /usr/bin/curl -O -C - "http://www.example.com/big-archive.zip"; export ec=$?; done

