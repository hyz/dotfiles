
import sys

if getattr(sys.version_info, 'major', 2) < 3:
    import urllib
    quote = urllib.quote
else:
    import urllib.parse
    quote = urllib.parse.quote

for line in open(sys.argv[1]):
    s = [ quote(x.strip()) for x in line.split('\t') ]
    if len(s) != int(sys.argv[2]):
        sys.stderr.write("#! %s\n" % line)
        continue
    print(sys.argv[3].format(*s))

