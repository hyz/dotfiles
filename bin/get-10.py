#!/usr/bin/env python3
#coding:utf-8
import sys, os, time, random
import signal
import requests
from html.parser import HTMLParser
# import gzip
# import json
# import re

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
            if self.stacks:
                x,y,_ = self.stacks[-1]
                self.stacks[-1] = (x,y,data.strip())
        def handle_endtag(self, tag):
            _,attrs,data = self.stacks[-1]
            if tag == 'input' and getval(attrs, 'name') == 'hdRpt':
                self.results[0] = getval(attrs,'value') #self.temps.append( self.stacks[:] )
            elif len(self.stacks) >= 4:
                t,a,d = self.stacks[-4] # table [('class', 'srgc'), ('id', 'srgca')] 
                if t == 'table' and tag == 'td' and data and getval(a,'id') in ('srgcc','srgca','srgcb'):
                    self.temps.append( self.stacks[:] )
                    #print(len(self.temps, self.temps))
            if tag == 'tr' and p.temps:
                self.results[1].append([ x[-1][-1] for x in p.temps ])
                p.temps = []
            self.stacks.pop()
    p = MyHTMLParser()
    p.stacks, p.temps, p.results = [], [], {1:[]}
    p.feed(html)

    for x,y in p.results.items():
        if x==0:
            print(y)
            continue
        for g in y: # '亿' '万'
            if len(g) == 4:
                a,b,c,d = g
                if b.endswith('万'):
                    n,_,_ = b.partition('万')
                    b = int( float(n) )
                else:
                    n,_,_ = b.partition('亿')
                    b = int( float(n)*10000 )
                s = '%06d %d %s' % (code,market, _NAMES.get(code))
                print(s, c.strip('%'),d.strip('%'), b, a, sep='\t')
                continue
            print(g)

    p.close()
    #first = re.compile('</span><span class="time-tip first-tip"><span class="tip-content">(.*?)</span>')
    #second = re.compile('</span><span class="time-tip"><span class="tip-content">(.*?)</span>')
    #third = re.compile('</span><span class="time-tip last-second-tip"><span class="tip-content">(.*?)</span>')
    #last = re.compile('</span><span class="time-tip last-tip"><span class="tip-content">(.*?)</span>')
    #visit.extend(first.findall(html))
    #visit.extend(second.findall(html))
    #visit.extend(third.findall(html))
    #visit.extend(last.findall(html))

# http://m.10jqka.com.cn/stockpage/hs_600460/
# http://basic.10jqka.com.cn/mobile/600338/profilen.html
# http://basic.10jqka.com.cn/mobile/600460/companyn.html
#
#Host: basic.10jqka.com.cn
#_URL     = os.environ['EM_URL']
#_REFERER = os.environ['EM_REFERER']
_URL = 'http://basic.10jqka.com.cn/mobile/%(code)06d/profilen.html'
_URL = 'http://basic.10jqka.com.cn/mobile/%(code)06d/companyn.html'
_REFERER = 'http://m.10jqka.com.cn/stockpage/%(market)s_%(code)06d/'
_Marks = ('sz', 'hs')

def GET(session, code, market, outfilename):
    fmtd = { 'code':code, 'market':_Marks[market] } # locals()
    url = _URL % fmtd
    headers = {
        "User-agent":"Mozilla/5.0 (Macintosh; Intel Mac OS X 10.10; rv:36.0) Gecko/20100101 Firefox/36.0",
        "Referer": _REFERER % fmtd,
        "Accept":"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        "Accept-Language":"en-US,zh-CN;q=0.8,en;q=0.5,zh;q=0.3",
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

def parse():
    global _NAMES
    def sdic():
        names = {}
        with open(os.path.join(HOME,'._sname'), encoding='gbk') as sf:
            for x in sf:
                v = x.strip().split(None,2)
                names[ int(v[0]) ] = v[-1]
        return names
    _NAMES = sdic()

    top = '.'
    if len(sys.argv) > 1:
        top = sys.argv[1]
    for top, dirs, files in os.walk( top ):
        for fn in files:
            v = fn.split('.')
            code,market = int(v[0]), int(v[1])
            with open( os.path.join(top,fn) ) as f:
                parse_html(code, market, f.read())
        break

def main():
    code, market = int(sys.argv[1]), int(sys.argv[2])
    session = requests.Session()
    GET(session, code,market, '%06d.%d.html' % (code,market))
    #download(sys.argv[1])

if __name__ == '__main__':
    _STOP = 0
    def sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    signal.signal(signal.SIGINT, sig_handler)
    try:
        main()
    except Exception as e:
        print(e, file=sys.stderr)

# >>> import requests
# >>> help(requests.get)
# >>>
# requests.get(url, cookies={'JSESSIONID':'abcI-FcxUP42naKy1qZsv'})
# post(..., data=json.dump({'email':'email','password':'pass'}) , )
#

