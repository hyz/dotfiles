#!/usr/bin/python

# /bin/ls -1 *.aac | ffmpeg-concat-aac.py /tmp/a.aac
## ffmpeg -i a.aac -y -acodec copy output.m4a

import os, sys

output = sys.argv[1]
input = '|'.join( line.strip() for line in sys.stdin )

cmd = f'ffmpeg -i "concat:{input}" -acodec copy {output}'
print("#", cmd)

os.popen(cmd)

