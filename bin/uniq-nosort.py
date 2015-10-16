import sys

ss = {}

for x in sys.stdin:
    x = x.strip()
    if x not in ss:
        ss[x] = 1
        print(x)
    else:
        ss[x] += 1

