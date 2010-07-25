#!/usr/bin/python3

import re, struct

data='ffff00022zZoOBB1vVAAMMMM2uuuuUUUUJJJJ'
desc='{4}4[1[{1}{1}]{2}]{4}1[{4}]{4}'

def pf(lv, desc, n, data):
    # print ('\t'*lv, '%s loop=%d' % (desc, n), sep=', ')
    if desc:
        for x in range(n):
            current, left = desc, None
            while 1:
                current, dx, left = re.match('(.*?)(\d\[|\])(.*)', current).groups()
                # print ('\t'*lv, current, dx, left, sep=', ')
                if current:
                    tmp = current.replace('{', '(.{')
                    tmp = tmp.replace('}', '})')
                    # print('\t'*lv, 'USE/CONSUME', current, tmp, sep=', ')
                    v = re.match(tmp+'(.*)', data).groups()
                    print('\t'*lv, v)
                    data = v[-1]
                if dx == ']' or not left:
                    break
                else:
                    n = int(dx[0])
                    tmp, data = data[:n], data[n:]
                    current, data = pf(lv+1, left, int(tmp), data)
        return left, data
    return None,0

def parse(desc, data):
    pf(0, desc + ']', 1, data)

if __name__ == '__main__':
    parse('1[1[{1}]]', '35hello5world3foo')
    print ('\t'*0 + data)
    print ('\t'*0 + desc)
    parse(desc, data) #pf(0, desc+']', 1, data)

# s = 'ffffffff'
# ''.join(map(lambda x: struct.pack('B', int(x,16)), [ s[x*2:x*2+2] for x in range(len(s)/2) ]))

