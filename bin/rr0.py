#!/usr/bin/python3

import re

data='ffff00022zZoOBB1vVAAMMMM2uuuuUUUUJJJJ'
desc='{4}4[1[{1}{1}]{2}]{4}1[{4}]{4}'

def pf(lv, desc, n, data):
    print ('\t'*lv, '%s loop=%d' % (desc, n), sep=', ')
    if desc:
        for x in range(n):
            current, left = desc, None
            while 1:
                current, dx, left = re.match('(.*?)(\d\[|\])(.*)', current).groups()
                print ('\t'*lv, current, dx, left, sep=', ')
                if current:
                    print('\t'*lv, 'USE/CONSUME:', current, sep=', ')
                    # use current
                    # consume data
                    pass
                if dx == ']' or not left:
                    break
                else:
                    current, data = pf(lv+1, left, int(dx[0]), data)
        return left, data
    return None,0

pf(0, desc+']', 1, data)

