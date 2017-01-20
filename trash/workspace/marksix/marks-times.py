#!/usr/bin/env python3
import sys

hcnt = {}
nte = 0
for fn in sys.argv[1:]:
    for l in open(fn):
        l = l.strip()
        if not l:
            continue
        nte += 1
        for x in l.split()[-1:]:
            try:
                hcnt[int(x)] += 1
            except:
                hcnt.setdefault(int(x),1)

for x,n in hcnt.items():
    print(x,n)
print(nte)

