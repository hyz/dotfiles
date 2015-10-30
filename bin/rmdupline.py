import sys, re

expr = re.compile(sys.argv[1])
prev = None

for line in sys.stdin:
    line = line.strip()
    if not re.match(expr, line):
        if prev:
            print(prev)
            prev = None
        print(line)
    else:
        prev = line

