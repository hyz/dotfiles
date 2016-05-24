#!/usr/bin/env python3
import sys

hcnt = {}
for fn in sys.argv[1:]:
    for l in open(fn):
        l = l.strip()
        if not l:
            continue
        for x in l.split():
            try:
                hcnt[int(x)] += 1
            except:
                hcnt.setdefault(int(x),1)

for x,n in hcnt.items():
    print(x,n)

