#!/usr/bin/env python

def rand_uniq(n, start=1000000000000000, stop=9999999999999999):
    import random
    h = {}
    random.seed()
    for x in range(n):
        while h.setdefault(random.randint(start, stop), x) != x:
            pass
    return h

if __name__ == '__main__':
    for x in rand_uniq(1000):
        print x

