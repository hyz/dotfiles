#!/bin/env python3
import sys, os

HOME= os.environ['HOME'];

def toutf8(Fn, encoding):
    with open(Fn, encoding=encoding) as f:
        lines = [ x.strip() for x in f ]
    print(*lines, sep='\n',end='\n',file=open(Fn,'w'))

def addname(Fn, Sf):
    names = {}
    with open(Sf, encoding='gbk') as sf:
        for x in sf:
            v = x.strip().split()
            names[v[0]] = v[-1]
    lines = []
    with open(Fn) as f:
        for x in f:
            x = x.rstrip()
            k = x.split(None,1)[0]
            lines.append(x +'\t'+ names.get(k,'Unknown'))
    print(*lines, sep='\n',end='\n',file=open(Fn,'w'))

if __name__ == '__main__':
    addname(sys.argv[1], os.path.join(HOME,'._sname'))

