
    nomino --test -p --regex '(.*) .20[0-9]+-[0-9]+-[0-9]+ [0-9]+_[0-9]+_[0-9]+ [AP]M..html$' '{1}.html'
    nomino --test -p --regex '(.*) .[0-9]+_[0-9]+_20[0-9]+ [0-9]+_[0-9]+_[0-9]+ [AP]M..html$' '{1}.html'

    ename --dry-run 's/【看破西游】(\d\d).mp3/\1.mp3/' 【看破西游】??.mp3


nomino -p --regex '.*》(.*)' '{1}.mp3'
