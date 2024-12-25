
# nomino https://github.com/yaa110/nomino

    nomino --test -p --regex '(.*) .20[0-9]+-[0-9]+-[0-9]+ [0-9]+_[0-9]+_[0-9]+ [AP]M..html$' '{1}.html'
    nomino --test -p --regex '(.*) .[0-9]+_[0-9]+_20[0-9]+ [0-9]+_[0-9]+_[0-9]+ [AP]M..html$' '{1}.html'

    nomino -p --regex '.*》(.*)' '{1}.mp3'

# rnr https://github.com/ismaelgv/rnr

Capture several named groups and swap them
Capture two digits as number.
Capture extension as ext.
Swap groups.

    rnr -f '(?P<number>\d{2})\.(?P<ext>\w{3})' '${ext}.${number}' ./*

Original tree

    .
    ├── file-01.txt
    ├── file-02.txt
    └── file-03.txt

Renamed tree

    .
    ├── file-txt.01
    ├── file-txt.02
    └── file-txt.03

# ename

    ename --dry-run 's/【看破西游】(\d\d).mp3/\1.mp3/' 【看破西游】??.mp3

