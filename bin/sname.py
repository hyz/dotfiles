#!/bin/env python3
import sys, os

HOME= os.environ['HOME'];

def toutf8(Fn, encoding):
    with open(Fn, encoding=encoding) as f:
        lines = [ x.strip() for x in f ]
    print(*lines, sep='\n',end='\n',file=open(Fn,'w'))

def sdic():
    names = {}
    with open(os.path.join(HOME,'_/_sname'), encoding='gbk') as sf:
        for x in sf:
            v = x.strip().split(None,2)
            names[v[0]] = v[-1]
    return names

def appname(names, ix):
    def getlines(f):
        lines = []
        for x in f:
            x = x.rstrip()
            fs = x.split('\t')
            k = fs[0].split()[0]
            fs.insert(abs(ix)<len(fs)and ix or len(fs), names.get(k,'Unknown') )
            lines.append('\t'.join(fs)) #(x +'\t'+ names.get(fields[0],'Unknown'))
        return lines

    outf = sys.stdout
    lines = []
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            lines = getlines(f)
        outf = open(sys.argv[1],'w')
    else:
        lines = getlines(sys.stdin)
    print(*lines, sep='\n',end='\n',file=outf)

if __name__ == '__main__':
    appname(sdic(), 0xfff)#(sdic(), -1)

