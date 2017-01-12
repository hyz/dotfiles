#!/usr/bin/env python3

import sys, time, os, re
import subprocess, shutil, glob, tempfile
import logging
import contextlib
import requests , json
from pprint import pprint

def log_level(lev):
    try:
        from http.client import HTTPConnection # py3
    except ImportError:
        from httplib import HTTPConnection # py2
    #logging.getLogger("requests").setLevel(logging.WARNING)
    #logging.getLogger("urllib3").setLevel(logging.WARNING)
    root_logger = logging.getLogger()
    root_logger.setLevel(lev)
    requests_log = logging.getLogger("requests.packages.urllib3")
    requests_log.setLevel(lev)
    if lev == logging.DEBUG:
        HTTPConnection.debuglevel = 1
        requests_log.propagate = True
    else:
        HTTPConnection.debuglevel = 0
        requests_log.propagate = False
        root_logger.handlers = []

@contextlib.contextmanager
def debug_requests():
    log_level(logging.DEBUG)
    yield
    log_level(logging.WARNING)

class Main(object):
    def __init__(self, *args, **kvargs):
        self.session = requests.Session()
        #self.cookies = requests.cookies.RequestsCookieJar()
    def GET(self, url, **kvargs):
        return self.session.get(url, **kvargs)
    def POST(self, url, **kvargs):
        return self.session.post(url, **kvargs)

    def test1(self, *args, times=1, **kvargs):
        times = int(times)
        while times > 0:
            times -= 1
            js = {"firstname": "John", "lastname": "Doe", "password": "jdoe123"}
            r = self.POST('http://localhost:11000/pusheen', data=json.dumps(js))
            print(r.json())

    def test2(self, *args, **kvargs):
        pass

def help(*args, **kvargs):
    print('Usages:'
            '\t{0} X=<XValue> Y=<YValue> test1 yarg\n'
            '\t{0} X=<XValue> Y=<YValue> test2 xarg yarg\n'
            .format(sys.argv[0]))
def main(fn, args, kvargs):
    logging.basicConfig(level=logging.DEBUG)
    #logging.getLogger().setLevel()
    t0 = time.time()
    m = Main(*args, **kvargs);
    getattr(m, fn, help)(*args, **kvargs)
    print('\ntime({}): {}"{}'.format(fn, *map(int, divmod(time.time() - t0, 60))) )

if __name__ == '__main__':
    def _fn_lis_dic(args):
        fn, lis, dic = None, [], {} # defaultdict(list)
        for a in args:
            if a.startswith('-'):
                assert ( '=' in a )
                a = a.strip('-')
            if '=' in a:
                k,v = a.split('=',1)
                v0 = dic.setdefault(k,v)
                if v0 is not v:
                    if type(v0) == list:
                        v0.append(v)
                    else:
                        dic[k] = [v0, v]
            else:
                if fn == None:
                    fn = a
                else:
                    lis.append(a)
        return fn, lis, dic
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    try:
        import signal
        signal.signal(signal.SIGINT, _sig_handler)
        main(*_fn_lis_dic(sys.argv[1:]))
    except Exception as e:
        help() #print(e, file=sys.stderr)
        raise

#>>> requests.get('http://httpbin.org/')
#<Response [200]>
#
#>>> debug_log_on()
#>>> requests.get('http://httpbin.org/')
#INFO:requests.packages.urllib3.connectionpool:Starting new HTTP connection (1): httpbin.org
#DEBUG:requests.packages.urllib3.connectionpool:"GET / HTTP/1.1" 200 12150
#send: 'GET / HTTP/1.1\r\nHost: httpbin.org\r\nConnection: keep-alive\r\nAccept-
#Encoding: gzip, deflate\r\nAccept: */*\r\nUser-Agent: python-requests/2.11.1\r\n\r\n'
#reply: 'HTTP/1.1 200 OK\r\n'
#header: Server: nginx
#...
#<Response [200]>
#
#>>> debug_requests_off()
#>>> requests.get('http://httpbin.org/')
#<Response [200]>
#
#>>> with debug_requests():
#...     requests.get('http://httpbin.org/')
#INFO:requests.packages.urllib3.connectionpool:Starting new HTTP connection (1): httpbin.org
#...
#<Response [200]>
#
#userdata = {"firstname": "John", "lastname": "Doe", "password": "jdoe123"}
#resp = requests.post('http://www.mywebsite.com/user', data=userdata)
#resp.json()

### http://stackoverflow.com/questions/2018026/what-are-the-differences-between-the-urllib-urllib2-and-requests-module?rq=1I

