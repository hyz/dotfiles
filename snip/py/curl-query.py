#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys, os, re
import subprocess, shlex
# import copy

def macaddr(host):
    import socket
    host = socket.gethostbyname(host)
    output = subprocess.Popen(['ip', 'route', 'get', host], stdout=subprocess.PIPE).communicate()[0]
    return open('/sys/class/net/{0}/address'.format( *re.findall('dev (\w*)', output) )).read().strip()
    #// return re.findall('dev (\w*) ', output)[0]

MY_ID=os.getenv('MY_ID', 1)
MY_USER=os.getenv('MY_USER', os.getenv('USER'))
MY_PASS=os.getenv('MY_PASS', '1234' + MY_USER)
MY_HOST=os.getenv('MY_HOST', '127.0.0.1')
MY_PORT=os.getenv('MY_PORT', '9000')
MY_PLAT=os.getenv('MY_PLAT', 'PY')

MY_ETHER=macaddr(MY_HOST)
MY_DIR=os.path.join(os.path.dirname(os.path.relpath(sys.argv[0])), MY_USER)
MY_COOK=os.getenv('MY_COOK', 'cook')
#MY_GWID=KK1007A

def myvar(k, d=None):
    # myvars = locals()
    # myvars.update( (k,v) for k,v in globals().items() if k.startswith('MY_') )
    return globals().get(k, d)

def query(method, path, params):
    _PATH = '?'.join([path, '&'.join(k+'='+v for k, v in params.items())])
    _URL = "http://{HOST}:{PORT}/{PATH}".format(PATH=_PATH, HOST=MY_HOST, PORT=MY_PORT)
    _COOK = os.path.join(MY_DIR, MY_COOK)
    _OUT_FN = os.path.join(MY_DIR, path.replace('/', '_'))
    curl = 'curl -# -4 -A cURL -c {_COOK} -b {_COOK}' \
                ' --dump-header {_OUT_FN}.header --output {_OUT_FN} {_URL}'.format(**locals())
    args = shlex.split(curl) #[ x.format(**locals()) for x in curl.split() ]
    if method == 'POST':
        args.append('--data-binary "@-"')
    ret = subprocess.Popen(args).wait()
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
        print ( ' '.join(args) )
        print (ret)
        # import pprint
        # pprint.pprint (locals())
    print (_URL)
    return ret

GET = lambda path, params: query('GET', path, params)
POST = lambda path, params: query('POST', path, params)

def login(path, params):
    params['macAddress'] = myvar('MY_ETHER','0')
    params.setdefault('phone', myvar('MY_USER'))
    params.setdefault('password', myvar('MY_PASS'))
    return query('GET', path, params)

### ### ### ### ### ### ### ### ### ### ### ###
# GET /foo/bar ...name=value
if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(3)
    params = dict(kv.split('=', 1) for kv in sys.argv[3:])
    func = globals().get(sys.argv[1])
    sys.exit( func(sys.argv[2].strip('/ '), params) )

# #. $MY_DIR/config
# #[ -n "$MY_USER" ] || . $MY_DIR/config
# #[ -n "$MY_COOKIE" ] || MY_COOKIE=cookie
# 
# # JSV=$MY_DIR/../bin/jvalue
# # [ -x "$JSV" ] || exit 9
# 
# TMP_DIR=$MY_DIR/$MY_USER
# CK=$TMP_DIR/$MY_COOKIE
# 
# [ -d "$TMP_DIR" ] || mkdir -p $TMP_DIR
# 
# Presult() {
#     local meth=$1
#     local url=$2
#     local path=$3
#     local leaf=$4
# 
#     echo "$meth $url" |tee -a $TMP_DIR/${leaf}
#     if ! grep 'HTTP/1.1 200' "$TMP_DIR/${leaf}"; then # if [ -z "$2" ]; then
#         # errno=`$JSV "$TMP_DIR/${leaf}.G" errno`
#         # if [ -n "$errno" -a "$errno" != "0" ]; then
#             cat "$TMP_DIR/${leaf}.$meth"
#             echo
#             echo "FAIL: $path; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
#             exit 1
#         # fi
#     # else echo "${leaf}.G $2"
#     fi
#     ls -l $TMP_DIR/$leaf*
# }
# 
# GET() {
#     local url="$1?"
#     local path=`echo $url |cut -d '?' -f1`
#     local qs=`echo $url |cut -d '?' -f2`
#     local leaf=`basename "$path"`
#     local url="http://$MY_HOST:$MY_PORT$path?$qs&PLAT=$MY_PLAT"
# 
#     echo "curl -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args $url"
#     curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args "$url"  > "$TMP_DIR/${leaf}.G"
#     if [ "$?" != "0" ]; then
#         echo "FAIL: $MY_PLAT $?; $url; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
#         exit 2
#     fi
#     Presult 'G' $url $path $leaf
# }
# 
# POST() {
#     #if [ -n "$MY_GWID" ]; then
#     #    Extra_Args="-H mac:$MY_ETHER -H gwid:$MY_GWID"
#     #fi
# 
#     local url="$1?"
#     local path=`echo $url |cut -d '?' -f1`
#     local qs=`echo $url |cut -d '?' -f2`
#     local leaf=`basename "$path"`
#     local url="http://$MY_HOST:$MY_PORT$path?$qs&PLAT=$MY_PLAT"
# 
#     echo "curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args $url --data-binary @-"
#     curl -# -4 -A $MY_PLAT -c $CK -b $CK -D $TMP_DIR/${leaf} $Extra_Args "$url" --data-binary "@-" > "$TMP_DIR/${leaf}.P"
#     if [ "$?" != "0" ]; then
#         echo "FAIL: $MY_PLAT $?; $url; $MY_USER $MY_PASS $MY_HOST:$MY_PORT"
#         exit 2
#     fi
#     Presult 'P' $url $path $leaf
# }

# curl -A $MY_PLAT -c /tmp/$USER.cj -b /tmp/$USER.cj $GP http://$MY_HOST$path --data-binary "$*"
