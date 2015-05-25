#!/usr/bin/python

import string
import time

def convert(line):
    ret=[]
    L=string.split(line)
    for s in L:
        if 10==len(s):
            t=time.strftime('%H:%M:%S', time.localtime(float(s)))
            ret.append(t)
        else:
            ret.append(s)

    if ( 'ONL'==L[-1] and 4 == len(L) ):
	ret.append( str( int(L[-2]) - int(L[-3]) ) )
    if ('MSG' == L[-1] and 6 == len(L)):
	ret.append( str(int(L[-2]) - int(L[-4])) )

    if ret:
        return string.join(ret)
    else:
        return ''

if __name__=='__main__':
    import sys
    if len(sys.argv)<2:
        print 'Usage:exec inputfile [outputifle]...'
        sys.exit(0)

    inf=sys.argv[1]
    if 3==len(sys.argv):
        sys.stdout=open(sys.argv[2],'w')

    fih = open(inf)
    for line in fih.readlines():
        outstr=convert(line)
        if len(outstr)>0:
  		    print outstr

    sys.stdout.close()
    fih.close()
