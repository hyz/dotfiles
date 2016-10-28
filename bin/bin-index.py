#!/usr/bin/python -u

import sys

heystack = open(sys.argv[1], 'rb').read()
if len(sys.argv) > 2:
    needle = open(sys.argv[2], 'rb').read()
else
    needle = sys.stdin.read()

print(heystack.index(needle))

