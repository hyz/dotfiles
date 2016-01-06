#!/bin/env python3

import sys

sd = {}

for l in open(sys.argv[1]):
    v = l.strip().split('\t')
    v[2:] = [float(x) for x in v[2:]]
    sd[int(v[0])] = v

for fn in sys.argv[2:]:
    for l in open(fn):
        v = l.strip().split('\t')
        v[2:] = [float(x) for x in v[2:]]
        p = sd.get(int(v[0]))
        if not p:
            continue
        p[2:] = [ x+y for x,y in zip(p[2:], [float(x) for x in v[2:]]) ]

for v in sd.values():
    print('{} {:.2f}'.format(v[0], (v[5]-v[7])/10000))

