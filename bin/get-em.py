#!/usr/bin/env python3
#coding:utf-8
import sys, os, time, random
import signal
import requests
from html.parser import HTMLParser
# import gzip
# import json
# import re

HOME = os.environ.get('HOME')

def read_names():
    names = {}
    with open(os.path.join(HOME,'_/_sname'), encoding='gbk') as sf:
        for x in sf:
            v = x.strip().split(None,2)
            names[ int(v[0]) ] = v[-1]
    return names

def getval(pairs, k):
    for x,y in pairs:
        if x==k:
            return y
    return None

def parse_html(code,market, html):
    class MyHTMLParser(HTMLParser):
        def handle_starttag(self, tag, attrs):
            self.stacks.append( (tag,attrs,'') )
        def handle_data(self, data):
            data = data.strip()
            if self.stacks and data:
                x,y,_ = self.stacks[-1]
                self.stacks[-1] = (x,y,data)
        def handle_endtag(self, tag):
            _,attrs,data = self.stacks[-1]
            if tag == 'input':
                if getval(attrs, 'name') == 'hdRpt':
                    self.hdRpt = getval(attrs,'value')
            elif tag == 'td' and data and self._scoped(-4):
                self.temps.append( data )
            elif tag == 'tr' and self.temps:
                srgc = self._scoped(-3)
                if srgc and len(self.temps) == 4:
                    self.results.setdefault(srgc,[]).append( self.prep(self.temps) )
                self.temps = []
            self.stacks.pop()
        def prep(self, lis):
            a,b,c,d = [ x.rstrip('-%') for x in lis ]
            m = 1
            if b.endswith('亿'):
                m = 10000
            elif b.endswith('万亿'):
                m = 10000 * 10000
            b = int( m*float(b.rstrip('-亿万') or '0') )
            c, d = float(c or '0'), float(d or '0')
            return (c, d, b, a)
            # print('%06d %d' % (code,market), c,d, b, a, k, sep='\t')
        def _scoped(self, x):
            if len(self.stacks) > abs(x):
                tag,attrs,_ = self.stacks[x]
                srgc = getval(attrs,'id')
                idcs = ('srgca','srgcb','srgcc')
                if tag == 'table' and srgc in idcs:
                    return idcs.index(srgc)+1
            return None
    par = MyHTMLParser()
    par.stacks, par.temps, par.results, par.hdRpt = [], [], {}, None
    par.feed(html)

    #print(par.hdRpt)
    for k,ds in par.results.items(): # '亿' '万'
        ea, ic, toks = 0.0, 0, []
        for a,b,c,s in sorted(ds, key=lambda v:v[0], reverse=True):
            ea += b * c
            ic += c
            if a > 10:
                toks.append('%.0f:%.0f:%s'%(a,b,s))
            #print('%06d %d %s' % (code,market,_NAMES.get(code)), c,d, b, a, k, sep='\t')
        print('%06d %d' % (code,market), k, int(10*ea/ic), ic, ';'.join(toks), sep='\t')

    par.close()
    #first = re.compile('</span><span class="time-tip first-tip"><span class="tip-content">(.*?)</span>')
    #second = re.compile('</span><span class="time-tip"><span class="tip-content">(.*?)</span>')
    #third = re.compile('</span><span class="time-tip last-second-tip"><span class="tip-content">(.*?)</span>')
    #last = re.compile('</span><span class="time-tip last-tip"><span class="tip-content">(.*?)</span>')
    #visit.extend(first.findall(html))
    #visit.extend(second.findall(html))
    #visit.extend(third.findall(html))
    #visit.extend(last.findall(html))

def GET(session, code, market, outfilename):
    _URL     = os.environ['EM_URL']
    _REFERER = os.environ['EM_REFERER']

    fmtd = { 'code':code, 'market':1+(market==0) } # locals()
    url = _URL % fmtd
    headers = {
        "User-agent":"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.10; rv:36.0) Gecko/20100101 Firefox/36.0",
        "Referer": _REFERER % fmtd,
        "Accept":"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        "Accept-Language":"en-US,en;q=0.5",
        "Accept-Encoding":"gzip, deflate",
        "Connection":"keep-alive",
        "Content-Type":"application/x-www-form-urlencoded",
    }

    rsp = session.get(url, params={}, headers=headers)

    print('=', rsp.status_code, code,market, rsp.url)
    if rsp.status_code == 200:
        print(' encoding', rsp.encoding)
        for x,y in rsp.cookies.items():
            print(' cookie', x,y)
        print(' Content-Encoding', rsp.headers.get('Content-Encoding'))

        with open(outfilename, 'w') as f:
            f.write(rsp.text)
        #parse_html(code,market, rsp.text)

def download(filename):
    lis = []
    for line in open(filename):
        v = line.split()
        code,market = int(v[0]), int(v[1])
        if not os.path.exists('%06d.%d.html' % (code,market)):
            lis.append( (code,market) )
    if len(lis) > 0:
        print()
        print(time.ctime(), 'count', len(lis))

        random.shuffle(lis)
        session = requests.Session()
        for code,market in lis:
            GET(session, code,market, '%06d.%d.html' % (code,market))

            time.sleep( random.randint(1,5) )
            if _STOP:
                break
            if random.randint(1,100) >70:
                session = requests.Session()

def parse(fp):
    bn,ext = os.path.splitext( fp )
    if ext != '.html':
        return
    code,market = [ int(x) for x in os.path.basename(bn).split('.') ]
    with open( fp ) as f:
        try:
            parse_html(code, market, f.read())
        except Exception as e:
            print('parse', fp, 'fail:', e, file=sys.stderr)

def each_file(path):
    if os.path.isdir(path):
        for top, dirs, files in os.walk( path ):
            for bn in files:
                yield os.path.join(top,bn)
            break
    else:
        yield path

def main():
    #for top, dirs, files in os.walk( len(sys.argv)>1 and sys.argv[1] or '.' ):
    #    parse(top,files)
    #    break
    for fp in each_file( len(sys.argv)>1 and sys.argv[1] or '.' ):
        parse(fp)
    #download(sys.argv[1])

if __name__ == '__main__':
    _STOP = 0
    def sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    signal.signal(signal.SIGINT, sig_handler)
    try:
        _NAMES = read_names()
        main()
    except Exception as e:
        print(e, file=sys.stderr)

# >>> import requests
# >>> help(requests.get)
# >>>
# requests.get(url, cookies={'JSESSIONID':'abcI-FcxUP42naKy1qZsv'})
# post(..., data=json.dump({'email':'email','password':'pass'}) , )
#
###
