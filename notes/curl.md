
http://curl.haxx.se/

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

