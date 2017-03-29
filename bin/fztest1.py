#!/usr/bin/env python3

import sys, time, os, re, random
import subprocess, shutil, glob, tempfile
import inspect # import currentframe, getframeinfo
import logging
import contextlib
import requests, socket, json
from pprint import pprint

def logging_level(lev):
    try:
        from http.client import HTTPConnection # py3
    except ImportError:
        from httplib import HTTPConnection # py2
    #logging.getLogger("requests").setLevel(logging.WARNING)
    #logging.getLogger("urllib3").setLevel(logging.WARNING)
    logging.basicConfig()
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
    logging_level(logging.DEBUG)
    yield
    logging_level(logging.WARNING)

def _query(numb, qf, url, quiet=0, timeout=15, headers={}, **kvargs):
    if not quiet:
        print('[{}] GET'.format(numb), url, 'T', timeout)
        for k,v in headers.items():
            print(' '*4,k,': ',v, sep='')
        print('[{}]'.format(numb), end=' ')
    try:
        resp = qf(url, timeout=int(timeout), headers=headers, **kvargs)
        if not quiet:
            print(resp.status_code)
            for k,v in resp.headers.items():
                print(' '*4,k,': ',v, sep='')
        return resp
    except (requests.exceptions.ReadTimeout,requests.packages.urllib3.exceptions.ReadTimeoutError,socket.timeout):
        pass
    return None

class Main(object):
    Length_DevUUID = 16

    def __init__(self, *args, **kvargs):
        self.session = requests.Session()
        self.get_numb = 0
        self.post_numb = 0
        #self.cookies = requests.cookies.RequestsCookieJar()

    #@contextlib.contextmanager
    def _GET(self, url, **kvargs):
        numb = self.get_numb
        self.get_numb += 1
        return _query(numb, self.session.get, url, **kvargs)
    def _POST(self, url, **kvargs):
        numb = self.post_numb
        self.post_numb += 1
        return _query(numb, self.session.post, url, **kvargs)

    def b1(self, *args, times=1, xtag=0, timeout=15, **kvargs):
        times = int(times)
        while times > 0 and not getattr(module,'STOP',None):
            #print('{0.filename}@{0.lineno}:'.format(inspect.getframeinfo(inspect.currentframe())))
            times -= 1
            # with debug_requests():
            headers={'X-Tag':str(xtag), 'Connection':'Keep-Alive'}
            r = self._GET(self.url(), timeout=timeout , headers=headers)
            if not r:
                continue
            print(r.text) # print(r.json())
            xtag = int( r.headers.get('X-Tag',xtag) )

    a_listdata = ( [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
                 + [13,14,15,16,17,18,19,20,21,22]
                 + [26,27,28,29,30,31,32,33,34,35]
                 + [39,40,41,42,43,44,45,46,47,48] ) * 2
    # = [ int(x)+1 for x in range(20) ] * 4 #+ [0,0]
    defa_devuuid = '0'*Length_DevUUID
    defa_ipaddress = '104.224.159.76'

    def url(self, DevUUID=None, IPAddress=None):
        if not DevUUID:
            DevUUID = self.defa_devuuid
        if not IPAddress:
            IPAddress = self.defa_ipaddress
        return 'http://{}:9022/2/{}'.format(IPAddress, DevUUID)

    def a1(self, *args, times=1, **kvargs):
        times = int(times)
        while times > 0 and not getattr(module,'STOP',None):
            times -= 1
            js = list(self.a_listdata);
            random.shuffle( js )
            headers={'Connection':'Keep-Alive'}
            #with debug_requests():
            #print('{0.filename}@{0.lineno}:'.format(inspect.getframeinfo(inspect.currentframe())))
            r = self._POST(self.url() , headers=headers , data=json.dumps(js))
            #print(dir(r))
            print('-'*40)
            #print('{0.filename}@{0.lineno}:'.format(inspect.getframeinfo(inspect.currentframe())))

    def a2(self, *args, times=1, DevUUID=None, **kvargs):
        assert len(DevUUID)==self.Length_DevUUID
        times = int(times)
        while times > 0 and not getattr(module,'STOP',None):
            times -= 1
            js = list(self.a_listdata);
            random.shuffle( js )
            headers={'Connection':'Keep-Alive'}
            r = self._POST(self.url(DevUUID=str(random.randint(0,9))*self.Length_DevUUID)
                    , data=json.dumps(js)
                    , headers=headers
                    , quiet=1)
            #print('-'*40)

def help(*args, **kvargs):
    print('Usages:'
            '\t{0} X=<XValue> Y=<YValue> a1 yarg\n'
            '\t{0} X=<XValue> Y=<YValue> b1 xarg yarg\n'
            .format(sys.argv[0]))

def _main_():
    def _fn_lis_dic(args):
        fn, lis, dic = '', [], {} # defaultdict(list)
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
                if not fn:
                    fn = a
                else:
                    lis.append(a)
        return fn, lis, dic
    def _fn(fn):
        f = getattr(module, fn, None)
        if not f:
            cls = getattr(module, 'Main', lambda *x,**y: None)
            f = getattr(cls(*args, **kvargs), fn, None)
        if not f:
            f = getattr(module, 'help', None)
        if not f:
            raise RuntimeError(fn, 'not found')
        return f
    t0 = time.time()
    fn, args, kvargs = _fn_lis_dic(sys.argv[1:])
    _fn(fn)(*args, **kvargs)
    sys.stdout.flush()
    print('time({}): {}m{}s'.format(fn, *map(int, divmod(time.time() - t0, 60))), file=sys.stderr)

if __name__ == '__main__':
    module = sys.modules[__name__]
    def _sig_handler(signal, frame):
        setattr(module, 'STOP', 1)
    try:
        import signal
        signal.signal(signal.SIGINT, _sig_handler)
        _main_()
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

