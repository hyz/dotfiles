#!/usr/bin/python
import os, sys

def copy_n(src, dst, n):
    while n > 0:
        dst.write( src.read(min(n,1024*8)) )
        n -= 1024*8

clen = int(os.environ.get('CONTENT_LENGTH','0'))
if clen > 0:
    with open('data.post','wb') as f:
        copy_n(sys.stdin.detach(), f, clen) # shutil.copyfileobj(sys.stdin, f)

content = '\n'.join('%s: %s' % (x,y) for x,y in os.environ.items())
sys.stdout.write('Content-Length: %s\r\n\r\n' % len(content))
sys.stdout.write(content)

