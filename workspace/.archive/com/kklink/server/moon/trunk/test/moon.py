#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, re, subprocess # ,copy

MY_USER=os.getenv('MY_USER', os.getenv('USER'))
MY_PASS=os.getenv('MY_PASS', '1234' + MY_USER)
MY_HOST=os.getenv('MY_HOST', '127.0.0.1')
MY_PORT=os.getenv('MY_PORT', '9000')
MY_XDIR=os.path.join(os.getenv('MY_DIR','.'), MY_USER)
def get_uid(user):
    p = os.path.join(MY_XDIR, 'cook')
    if os.path.exists(p):
        for l in open(p):
            #yx.kklink.com   FALSE   /       FALSE   0       MT      0d003d8dfbbca52929f9
            v = l.split('\t')
            if len(v) < 7:
                continue
            h, t = v[0], v[6]
            if h == MY_HOST:
                #print ((h,MY_HOST,t[8:]))
                return int(t[16:],16)
    return 0
MY_UID=get_uid(MY_USER)

def uidpair(p, params):
    x,y = MY_UID, int(params['to'])
    if y < x:
        x,y = y,x
    dot = params['dot']
    return dot + str(x) + dot + str(y)

def macaddr(host):
    import socket
    host = socket.gethostbyname(host)
    output = subprocess.Popen(['ip', 'route', 'get', host], stdout=subprocess.PIPE).communicate()[0]
    return open('/sys/class/net/{0}/address'.format( *re.findall('dev (\w*)', output) )).read().strip()
    #// return re.findall('dev (\w*) ', output)[0]

# MY_ID=os.getenv('MY_ID', 0)
# MY_GWID=KK1007A
MY_PLAT=os.getenv('MY_PLAT', 'PY')
MY_ETHER=macaddr(MY_HOST)
MY_COOK=os.getenv('MY_COOK', 'cook')
#MY_DIR=os.path.join(os.getenv('MY_DIR',None) or os.path.dirname(os.path.relpath(sys.argv[0])), MY_USER)

def env(k, d=None): return globals().get(k, d)

def query(method, path, params):
    _PATH = '?'.join([path, '&'.join(k+'='+v for k, v in params.items())])
    _URL = "http://{HOST}:{PORT}/{PATH}".format(PATH=_PATH, HOST=MY_HOST, PORT=MY_PORT)
    _COOK = os.path.join(MY_XDIR, MY_COOK)
    _OUT_FN = os.path.join(MY_XDIR, path.replace('/', '_'))
    curl = 'curl -# -4 -A cURL -c {_COOK} -b {_COOK}' \
                ' --dump-header {_OUT_FN}.header --output {_OUT_FN} {_URL}' #.format(**locales())
    subps = [ x.format(**locals()) for x in curl.split() ]
    if method == 'POST':
        subps += '--data-binary @-'.split()
    ret = subprocess.Popen(subps).wait()
    if ret == 0:
        sline = open(_OUT_FN + '.header').readline().strip()
        print (sline)
        scode = sline.split()[1]
        if scode != '200':
            print ( open(_OUT_FN).read() )
            ret = int(scode)
            print (ret)
        print (_OUT_FN)
    else:
        print ( ' '.join(subps) )
        print (ret)
        # import pprint
        # pprint.pprint (locals())
    print (_URL)
    return ret

GET = lambda path, params: query('GET', path, params)
POST = lambda path, params: query('POST', path, params)

# def LOGIN(path, params):
#     params['macAddress'] = env('MY_ETHER','0')
#     params.setdefault('phone', env('MY_USER'))
#     params.setdefault('password', env('MY_PASS'))
#     return query('GET', path, params)

### ### ### ### ### ### ### ### ### ### ### ###
# GET /foo/bar ...name=value
# 1   2        ...name=value
if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(3)
    # print(sys.argv)
    params = dict( x.format(**globals()).split('=', 1) for x in sys.argv[3:] )
    params['macAddress'] = env('MY_ETHER','0')
    params['PLAT'] = env('MY_PLAT','')
    hf = globals().get(sys.argv[1])
    sys.exit( hf(sys.argv[2].strip('/ '), params) )

