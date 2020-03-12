import os, sys, glob
from mutagen.id3 import ID3, TIT2

def main(top):
    print(top)
    for fp in glob.glob(os.path.join(top, '*.mp3')):
        tit, _ = os.path.splitext(os.path.basename(fp))
        tit, _ = tit.split(None,1)
        id3 = ID3(fp)
        id3.add(TIT2(encoding=3, text=tit))
        id3.pprint()
        break #id3.save()

if __name__ == '__main__':
    main(dict(enumerate(sys.argv)).get(1,'.'))

# https://mutagen.readthedocs.io/en/latest/user/id3.html
