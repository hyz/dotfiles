
    curl -C - -LO http://...

### ...

Test direct proxying with cURL:

    curl -kvx localhost:8080 http://www.google.com/humans.txt
    curl -kvx localhost:8080 https://www.google.com/humans.txt

Test HTTP connect with cURL:

    curl -kpvx localhost:8080 http://www.google.com/humans.txt
    curl -kpvx localhost:8080 https://www.google.com/humans.txt

json

    curl -H "Content-Type: application/json" -X POST -d '{"key": "value"}' http:localhost:18001/get

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

