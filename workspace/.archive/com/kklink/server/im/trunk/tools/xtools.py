#
#
# redis-cli <<<"hgetall users" |head |python gen-user-sign.py
#

import sys, os
import getopt
import random, json
from subprocess import Popen, PIPE

bstok = '0c4707dab8c186a7bcaffa58abeb2310'
profs = {'name':'wood','uid':10007,'head':'http://avatar.csdn.net/8/C/6/1_alula.jpg','token':'f50cf157b11374fae7afd01b62225090'}

def list_join(lines, N):
    ls, tmpls = [], []
    n = 0
    for line in lines:
        tmpls.append(line)
        n += 1
        if n == N:
            ls.append(tmpls)
            n = 0
    return ls

def _list_uidtok(rfile):
    lis = []
    for line in rfile:
        line = line.strip()
        if line.startswith('{') and line.endswith('}'):
            d = json.loads(line.decode('UTF-8'))
            if d['uid'] < 10000:
                continue
            if len(d['token']) == 0:
                continue
            lis.append( json.loads(line.decode('UTF-8')) )
    return lis
def _list_uidtok2(rfile):
    lis = []
    for line in rfile:
        line = line.strip()
        uid,tok = line.split()
        uid = int(uid)
        lis.append( {'uid':uid, 'token':tok} )
    return lis

def list_uidtok(*argv):
    for d in _list_uidtok(sys.stdin):
        print('{uid}\t{token}'.format(**d))

_fmt001 = '{uid}\t1\t{{}}'
_fmt099 = '{uid}\t99\t{{"version":"1.0.0","cmd":99,"body":{{"appid":"yx","uid":{uid},"token":"{token}"}}}}'
_fmt205 = '{uid}\t205\t{{"cmd":205,'\
        '"body":{{"data":{{"content":{{"text":{text}}},"type":1,'\
        '"from":{{"head":"{head}","uid":{uid},"name":"{name}"}}}},'\
        '"to":[{{"uid":{uid2},"name":"{name}"}}],"sid":"21/{uid2}"}},"appid":"yx","version":"1.0.0"}}'

def fmts_do(fmts, lis):
    for fmt in fmts:
        for d in lis:
            print (fmt.format(**d))

def rand205(*argv):
    lis = _list_uidtok2( sys.stdin )
    if len(argv) > 0:
        n = int(argv[0])
        if n > 0 and n < len(lis):
            x = random.randint(0,len(lis)-n-1)
            lis = lis[x:x+n]

    fmts_do([_fmt001, _fmt099], lis)

    text = Popen(["fortune"], stdout=PIPE).communicate()[0]
    text = json.dumps(text)

    nmsg = len(lis)
    if len(argv) > 1:
        nmsg = int(argv[1])
    while nmsg > 0:
        nmsg -= 1
        src, dst = random.sample(lis, 2)
        uid2 = dst['uid']
        profs.update( src )
        print (_fmt205.format(uid2=uid2, text=text, **profs))

def fmt99(*argv):
    for line in sys.stdin:
        line = line.strip()
        jd = json.loads(line.decode('UTF-8'))
        print (_fmt001.format(**jd))
        print (_fmt099.format(**jd))

# http://avatar.csdn.net/8/C/6/1_alula.jpg
def fmt205(*argv):
    text = sys.stdin.read().strip()
    for uid2 in argv:
        print (_fmt205.format(uid2=uid2, text=text, **profs))

#75351 522345 165 205 TRACE
def microdiff():
    h = {}
    for l in sys.stdin:
        mid, mic, _ = l.split(None,2)
        mid, mic = int(mid), int(mic)
        h.setdefault(mid, []).append(mic)
    for mid, l in h.items():
        if len(l) == 3:
            x = l[1] - l[0]
            y = l[2] - l[1]
            z = l[2] - l[0]
            print('{0}\t{1}\t{2}\t{3}'.format(mid, x,y,z))

def main():
    fx = globals().get(sys.argv[1])
    fx(*sys.argv[2:])

def gen_user_reg_json():
    fr_numb = 49990
    count = 10
    ofp = None

    if len(sys.argv) > 1:
        opts, args = getopt.getopt(sys.argv[1:],"ho:")
        opts = dict(opts)
        if '-h' in opts or '--help' in opts:
            print 'Usage: {0} -o <outs> <from-uid> <count>'.format(sys.argv[0])
            sys.exit(0)
        for o,v in opts.items():
            ofp = v
        if len(args):
            if len(args) != 2:
                sys.exit(1)
            fr_numb = int(args[0])
            count = int(args[1])
    
    f_ids = f_reg = f_sig = sys.stdout
    if ofp:
        f_ids = open(ofp + '.user', 'w')
        f_reg = open(ofp + '.reg', 'w')
        f_sig = open(ofp + '.sign', 'w')

    fmt_reg = '{0}\t111\t{{"version":"1.0.0","appid":"yx","cmd":111,"auth":"{1}","body":{{"uid":{0},"token":"={0}="}}}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_reg.write( fmt_reg.format(x, bstok) );

    fmt_sig = '{0}\t99\t{{"version":"1.0.0","cmd":99,"body":{{"appid":"yx","uid":{0},"token":"={0}="}}}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_sig.write( fmt_sig.format(x) );

    fmt_ids = '{0}\t1\t{{0:0}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_ids.write( fmt_ids.format(x) )

if __name__ == '__main__':
    main()


