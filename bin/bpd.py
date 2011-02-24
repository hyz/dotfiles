#!/usr/bin/env python

import sys, re, struct

hexize = lambda s: ''.join([ '%02x' % ord(x) for x in s ])
unhex = lambda s: ''.join([ chr(int(s[x:x+2],16)) for x in range(0, len(s), 2) ])
# hex2bytes = lambda s: bytes([ int(s[x:x+2],16) for x in range(0, divmod(len(s),2)[0]*2, 2) ])

def length_of(x, data):
    if x == 'N':
        x, = struct.unpack('!B', data[:1])
        data = data[1:]
    elif x == 'H':
        x, = struct.unpack('!H', data[:2])
        data = data[2:]
    elif x == 'L':
        x, = struct.unpack('!I', data[:4])
        data = data[4:]
    else:
        x = int(x)
    return x, data

def value_of(y, data):
    if y == 's':
        y = str(data)
    elif y == 'd':
        qs = ('', '!B', '!H', '', '!I', '', '', '', '!Q')
        y, = struct.unpack(qs[len(data)], data)
    else:
        y = hexize(data)
    return y, data

def consume_group_desc(desc):
    i, oc = 0, 1
    for i, x in enumerate(desc):
        if x == '(':
            oc += 1
        elif x == ')':
            oc -= 1
            if oc == 0:
                break
    return desc[:i+1], desc[i+1:]

def group(l, desc, n, data):
    desc, left = consume_group_desc(desc)
    for x in range(n):
        l.append([])
        _, data = consume(l[-1], desc, data)
    return left, data

def consume(l, cur, data):
    try:
        if not cur or cur[0] == ')':
            return cur and cur[1:], data
        cnt, y, cur = re.match('(\d+|N|H|L)([xds(])(.*)', cur.strip(', :;./+')).groups()
        cnt, data = length_of(cnt, data)
        if y == '(':
            cur, data = group(l, cur, cnt, data)
            return consume(l, cur, data)
        d, data = data[:cnt], data[cnt:]
        l.append(value_of(y, d))
        return consume(l, cur, data)
    except:
        raise RuntimeError(cur, data)

def parse(desc, data):
    l = []
    _, y = consume(l, desc, data)
    return l, y

def ofmt(l, width=0):
    if type(l) == list:
        if filter(lambda x: type(x)==list, l) or width==0:
            width += max(map(lambda e: type(e) != list and len(str(e[0])) or 0, l))
            map( lambda x: ofmt(x, width), l )
        else:
            print '%%%ds' % width % ','.join([str(x) for x,y in l])
    else:
        x, y = l
        print '%%%ds' % width % str(x), '#', hexize(y)

def xcode(desc, line):
    print '===', desc, '==='
    try:
        l, y=parse(desc, unhex(line))
        ofmt(l)
        if y: print hexize(y)
    except RuntimeError as e:
        x, y = e.args
        print 'Error', x[:16], hexize(y[:16])

if __name__ == '__main__':
    describes = {
               -102 : ('8x,Ms,4d,4d,4d', '4d,4d,4d'),
               -100 : ('4x,1d,4x,4x,4d,16x,Ns,2d,2d,Ns,Ns,Ns,4d', '1d,4x,4d'),
                 -1 : ('1d,4x,4x,4x,Ns,Nx,1d,2d,N(Ns,Nx,Nx,2d,1d),N(Ns,Nx,2d)', ''),
                  0 : ('4x,2d,4x,4x,15s,15x,Ns,4d,16x,Ns,2d,2d,8d,4x,4x,4d,4d', '2d,4x,4d,1x,8d,N(Nx)'),
               1001 : ('4x,Ns,4x,4x,4x,4x', '1x,8s,H(4d,1d,1d,4d,1d),N(1d,Ns),N(Ns),Ns'),
            }
    desc = describes.get(0)
    if len(sys.argv) > 1:
        desc = (sys.argv[1], sys.argv[2]) # tuple([ sys.argv[1] ] * 2)
    while 1:
        line = sys.stdin.readline().strip()
        if line:
            if line.strip('?') == '':
                for x,y in describes.items():
                    print '%4s' % str(x), y
                print '===', desc, '==='
            elif line.startswith('&'):
                desc = describes.get(int(line[1:].strip() or '0'), desc)
            elif line == '<>':
                if desc[1]:
                    desc = (desc[1], desc[0])
            elif line.startswith('='):
                desc = line.strip('= \t') #tuple([ line.strip('= \t"\'') ] * 2)
            elif line.startswith('!'):
                desc, line = line.strip('! \t').split()
                xcode(desc, line)
            else:
                xcode(desc[0], line.strip())

# xxd -g0 -p
# REQ Head:  4x,2d,4x,15x,15x,4x,16x,Ns,2d,2d,8x,4x,4d,4d
# RESP Head: 2d,4x,4d,1x,8d,N(Nx)
# 1001 REQ:  4x,Ns,Ns,4x,4x,4x,4x
# 1001 RESP: 1x,8x,H(4d,1d,1d,4d,1d),N(1d,Ns),N(Ns),Ns
# 1001 RESP DATA: 03E937C508DF0000000000000000004C8C71730040706B696E67616D650006000000020B0200000064050000000402020000006400000000060902000000000000000001010100000320000000000302020000006400000000050B02000000640A050B38C7EBCAB9D3C3D6D0B9FAD2C6B6AFCAD6BBFAB3E4D6B5BFA8B3E4D6B5A3ACB3E4D6B5C7B0D7D0CFB8BACBB6D4D5CBBAC5D3EBC3DCC2EBA3A10C38C7EBCAB9D3C3D6D0B9FAC1AACDA8CAD6BBFAB3E4D6B5BFA8B3E4D6B5A3ACB3E4D6B5C7B0D7D0CFB8BACBB6D4D5CBBAC5D3EBC3DCC2EBA3A10D1AC7EBD7D0CFB8BACBB6D4B3E4D6B5D5CBBAC5D3EBC3DCC2EBA1A30E1AC7EBD7D0CFB8BACBB6D4B3E4D6B5D5CBBAC5D3EBC3DCC2EBA1A30A2DBBB0B7D1B3E4D6B5B3C9B9A6BAF3CFB5CDB3BDABBBE1D4DAC4FABBB0B7D1D6D0BFDBB3FDCFE0D3A6BDF0B6EE210320B4F3CAA6C8FCBCB4BDABBFAAB7C5A3ACC7EBD7A2D2E2CFB5CDB3B9ABB8E6A3A122C3BFCFEEB1C8C8FCB6BCBBE1B3E9B3F6D0D2D4CBBDB1A3ACC4E3B2BBCFEBC4C3A3BF2AC3BFD4C2B6BCD3D0D0C2D3CECFB7CDC6B3F6A3ACBBB9B2BBBFECC8A5D3CECFB7B3A1C1B7C1B7CAD6A3BF00

