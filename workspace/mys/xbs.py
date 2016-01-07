#!/bin/env python3

import sys
# 2:code:date     1:0.3less       4:20bs       4:bs       4:ochl
#  #

def main():
    sdic = {}

    for l in open(sys.argv[1]):
        v = l.strip().split('\t')
        v[2:] = [float(x) for x in v[2:]]
        sdic[int(v[0])] = v

    for fn in sys.argv[2:]:
        for l in open(fn):
            v = l.strip().split('\t')
            v[2:] = [float(x) for x in v[2:]]
            s = sdic.get(int(v[0]))
            if not s:
                continue
            s[2:] = [ x+y for x,y in zip(s[2:], [float(x) for x in v[2:]]) ]

    for v in sdic.values():
        print(  v[0]
                ,'{:.2f}'.format((v[5]-v[7])/10000)
                , sep='\t')

main()

