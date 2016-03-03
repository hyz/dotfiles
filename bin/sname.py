#!/bin/env python3
import sys, os

HOME= os.environ['HOME'];

def toutf8(Fn, encoding):
    with open(Fn, encoding=encoding) as f:
        lines = [ x.strip() for x in f ]
    print(*lines, sep='\n',end='\n',file=open(Fn,'w'))

def sdic():
    names = {}
    with open(os.path.join(HOME,'._sname'), encoding='gbk') as sf:
        for x in sf:
            v = x.strip().split(None,2)
            names[v[0]] = v[-1]
    return names

def appname(names):
    def outfile():
        return (len(sys.argv)>1) and open(sys.argv[1],'w') or sys.stdout
    def getlines(f):
        lines = []
        for x in f:
            x = x.rstrip()
            k = x.split(None,1)[0]
            lines.append(x +'\t'+ names.get(k,'Unknown'))
        return lines

    lines = []
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            lines = getlines(f)
    else:
        lines = getlines(sys.stdin)

    print(*lines, sep='\n',end='\n',file=outfile())

if __name__ == '__main__':
    appname(sdic())

