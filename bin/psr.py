#!/usr/bin/python3

import sys, re, struct

data=b'ffff00022zZoOBB1vVAAMMMM2uuuuUUUUJJJJ-extra'
desc='{4}4[1[{1}{1}]{2}]{4}1[{4}]{4}'
desc='4,4[1[1,1],2],4,1[4],4'
desc='4x,2d,4x,16s,Ns,Mx,N(4d,Ns)'
desc='4x,M(N(1x,1x),2x),4x,N(4x),4x'


def pf(lv, desc, n, data):
    # print ('\t'*lv, '%s loop=%d' % (desc, n), sep=', ')
    # print ('RECURSIVE ENTER', desc)
    for x in range(n):
        current, left = desc, None
        while current:
            # print ('WHILE', current)
            current, nm, left = re.match('(.*?)([\dNM]*)[()](.*)', current).groups()
            # print('RE match', current, nm, left)
            for x in (current.strip(',') and current.strip(',').split(',')):
                x, y = x[:-1], x[-1]
                if x == 'N':
                    x, data = data[:1], data[1:]
                elif x == 'M':
                    x, data = data[:4], data[4:]
                d, data = data[:int(x)], data[int(x):]
                print('  '*lv, d)
            if nm:
                if nm == 'N':
                    nm, data = data[:1], data[1:]
                elif nm == 'M':
                    nm, data = data[:4], data[4:]
                current, data = pf(lv+1, left, int(nm), data)
                # print ('RECURSIVE RETURN', current)
            else:
                break
    return left, data

def parse(desc, data):
    x, y = pf(0, desc + ')', 1, data)
    if y:
        print ('Left: ', y)

if __name__ == '__main__':
    # print ('\t'*0 + str(data))
    # print ('\t'*0 + desc)
    # parse(desc, data) #pf(0, desc+']', 1, data)
    if len(sys.argv) > 1:
        print (sys.argv[1])
        while 1:
            line = sys.stdin.readline().strip()
            if line:
                parse(sys.argv[1], line)

# s = 'ffffffff'
# ''.join(map(lambda x: struct.pack('B', int(x,16)), [ s[x*2:x*2+2] for x in range(len(s)/2) ]))
# ''.join(map(lambda x: struct.pack('B', int(x,16)), [ s[x:x+2] for x in range(0,len(s),2) ]))

