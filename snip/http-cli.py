#!/bin/env python3

import sys, os
import http.client, urllib.parse
from pprint import pprint
import json

def getfirst(k, a, defa):
    for d in a:
        v = d.get(k, None)
        if not (v is None):
            return v
    return defa

def setdefault(dct, k, *a):
    for d in a:
        v = d.get(k, {})
        for x,y in v.items():
            dct.setdefault(x,y)

def response(resp, up):
    if resp.status != 200:
        print(resp.status, resp.reason, file=sys.stderr)
        for k,val in resp.getheaders():
            print(k, ':', val, file=sys.stderr)
        print(file=sys.stderr)
        print(resp.read(), file=sys.stderr)
        print(file=sys.stderr)
    headers = dict( resp.getheaders() )
    content = resp.read()
    return (resp.status, headers, content)

def request(path, params={}, headers={}, **vars):
    env = vars.get('env',{})

    setdefault(params, 'params', env, globals())
    setdefault(headers, 'headers', env, globals())

    conn = http.client.HTTPConnection( getfirst('host', (vars, env), '127.0.0.1') )
    conn.request(getfirst('method', (vars,env), 'GET'), path, urllib.parse.urlencode(params), headers)
    rsp = response(conn.getresponse(), locals())
    conn.close()
    return rsp

def getcookie(hdr):
    ck = hdr.get('Set-Cookie') # ('Set-Cookie', 'PHPSESSID=340v97bedvblr80l8o6o1nt1i4; expires=Thu, 14-Aug-2014 08:11:22 GMT; path=/')
    ck, _ = ck.split(';', 1)
    k, ck = ck.split('=')
    return (k,ck)

def getm(dct, *ks):
    ret = []
    for k in ks:
        ret.append( dct.get(k, None) )
    return ret

def main():
    fmt_ids = '{0}\t1\t{{0:0}}'
    fmt_sig = '{0}\t99\t{{"version":"1.0.0","cmd":99,"body":{{"appid":"yx","uid":{0},"token":"{1}"}}}}'
    for numb in range(14500000001,14500000061): # 14500000001,14500000060
        st, hdr, cont = request('/login', headers={'X-MOBILE':numb}, method='POST', env=globals())
        if st != 200:
            continue
        code, js = getm(json.loads(cont.decode('UTF-8')), 'code', 'data')
        if code != 0:
            print(code, js, file=sys.stderr)
            continue
        # pprint(js) # _,ck = getcookie(hdr)
        uid, tok, nick = getm(js, 'userid', 'token', 'nick')
        # print(uid, tok, nick)
        print(fmt_sig.format(uid, tok))
        print(fmt_ids.format(uid))

if __name__ == '__main__':
    params = {
        'password'  : '*',
        'bundleid'  : 'com.kklnk.yx',
        'autoLogin' : 0
    }
    headers = {
        'Content-Type'     : 'application/x-www-form-urlencoded',
        'X-APP-DEVICE'     : 2,
        'X-APP-LOGIN-TYPE' : 1
        'X-APP-VERSION'    : "3.0.1",
    }
    host = 'api.moon.kklink.com'
    main()

#curl -v -c cook -b cook http://api.moon.kklink.com/login \
#    -d password=* -d bundleid=com.kklnk.yx -d autoLogin=0 \
#    -H "X-APP-DEVICE: 2" -H "X-APP-VERSION: 3.0.1" -H "X-APP-LOGIN-TYPE: 1" -H "X-MOBILE: 14500000001"
#b'{"code":0,"data":{"userid":102812,"nick":" XL\xe7\xad\xb1\xe8\x93\x9d ","icon":"http:\\/\\/tx.tianyaui.com\\/logo\\/77639389","background":"","sex":"F","age":18,"constellation":"\xe5\x8f\x8c\xe5\xad\x90\xe5\xba\xa7","phone":"14500000001","mail":"","sign":"2014\xe8\xa6\x81\xe5\x8a\xa0\xe6\xb2\xb9........","fans_num":2,"attention_num":1,"birthday":"1996-06-14","money":30,"level":5,"rich":2530,"praiseNum":0,"renqi":451,"token":"20d62bf98881034868a9150ad1b9bb5e","blackList":[],"bindList":{"phone":"","qq":"","weibo":""},"IMServerConfigList":[{"ip":"58.67.160.243","port":10010}]}}'

