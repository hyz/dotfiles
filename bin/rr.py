#!/usr/bin/python3

import re, struct

data=b'ffff00022zZoOBB1vVAAMMMM2uuuuUUUUJJJJ'
desc='{4}4[1[{1}{1}]{2}]{4}1[{4}]{4}'
desc='4,4[1[1,1],2],4,1[4],4'
keyd = {(1,):'1-KEY=%s', (2,1,1): '211-KEY=%s', (2,2,2):'222-KEY=%s', (2,3):'23-KEY=%s', (3,):'3-KEY=%s', (4,1):'41-KEY=%s', (5,):'5-KEY=%s'}

def pf(lv, desc, n, data):
    # print ('\t'*lv, '%s loop=%d' % (desc, n), sep=', ')
    if desc:
        for x in range(n):
            current, left = desc, None
            lv[-1] = 0
            while 1:
                current, dp, left = re.match('(.*?)(\d\[|\])(.*)', current).groups()
                current = current.strip(', ')
                # print ('\t'*lv, current, dp, left, sep=', ')
                if current:
                    v = eval('[' + current + ']')
                    n = sum(v)
                    l = struct.unpack(''.join('%ds' % x for x in v), data[:n])
                    print('\t'*len(lv), end=' ')
                    for x in l:
                        lv[-1] += 1
                        print (keyd.get(tuple(lv), ''), end=' ')
                        print('%s:' % str(lv), str(x), end='')
                    print()
                    # print('\t'*len(lv), '%s:' % str(lv), ', '.join(str(x) for x in l))
                    data = data[n:]
                if dp == ']' or not left:
                    break
                else:
                    lv[-1] += 1
                    n = int(dp[0])
                    x, data = data[:n], data[n:]
                    current, data = pf(lv+[0], left, int(x), data)
        return left, data
    return desc,data

def parse(desc, data):
    pf([0], desc + ']', 1, data)

if __name__ == '__main__':
    #parse('1[1[{1}]]', '35hello5world3foo')
    print ('\t'*0 + str(data))
    print ('\t'*0 + desc)
    print (keyd)
    parse(desc, data) #pf(0, desc+']', 1, data)
    parse('1[1[1]]', b'35hello5world3foo')

# s = 'ffffffff'
# ''.join(map(lambda x: struct.pack('B', int(x,16)), [ s[x*2:x*2+2] for x in range(len(s)/2) ]))
# ''.join(map(lambda x: struct.pack('B', int(x,16)), [ s[x:x+2] for x in range(0,len(s),2) ]))

