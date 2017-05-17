#!/usr/bin/python
import sys, re

# [1494033662.691981] 64 bytes from 192.168.0.1: icmp_seq=235 ttl=64 time=41.3 ms

tims = []
tims1, tims_5, tims_2 = [], [], []
ignores = []

for line in sys.stdin:
    res = re.search('time=(\d+(\.\d+)?)', line)
    if not res:
        line = line.strip()
        if line:
            ignores.append(line)
        continue
    t = float(res.group(1))
    tims.append(t)
    if t >= 200:
        tims_2.append(t)
        if t >= 500:
            tims_5.append(t)
            if t >= 1000:
                tims1.append(t)

print('average/max/min:', sum(tims) / len(tims), max(tims), min(tims))
print('>=200' ,'count:', len(tims_2), float(len(tims_2))*100/len(tims), sep='\t')
print('>=500' ,'count:', len(tims_5), float(len(tims_5))*100/len(tims), sep='\t')
print('>=1000','count:', len(tims1) , float(len(tims1))*100/len(tims) , sep='\t')
print('total count:', len(tims))

for line in ignores:
    print('\t:', line, file=sys.stderr)

