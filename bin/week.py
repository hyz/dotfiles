import sys, time
from datetime import *

begin, end, delta = datetime(2017,1,1), datetime.today(), timedelta(days=1)
if len(sys.argv) > 1:
    begin = datetime.strptime(sys.argv[1], '%Y-%m-%d')
    if len(sys.argv) > 2:
        end = datetime.strptime(sys.argv[2], '%Y-%m-%d')
    print(begin, end, file=sys.stderr)

while begin <= end:
    _, wn, wd = begin.isocalendar()
    if wd in (5,):
        print( wn, wd )
    begin += delta

