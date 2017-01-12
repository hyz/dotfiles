#!/usr/bin/env python3

import sys, time, os, re
import subprocess, shutil, glob, tempfile
from pydub import AudioSegment, silence

def _parse_c_inc_file(infp):
    #{20000<<POS_SHIFT,20232<<POS_SHIFT},//0
    max_p = 0
    words = []
    with open(infp) as infile:
        idx = 0
        for line in infile:
            r = re.match('^[\s{(]+(\d+)[)<\s]*POS_SHIFT[,\s(]+(\d+)[)<\s]*POS_SHIFT[},\s/]*([\w]+).*$', line)
            if r:
                b,e,f = ( int(r.group(1)),int(r.group(2)), r.group(3) )
                if b < e:
                    if b < max_p:
                        raise TabError(line.strip())
                    max_p = e
                    words.append( (b,e,f) )
                    idx += 1
        # for t in words: print( *t, sep='\t' )
    return words

def _complete_parts(words, total_len=0):
    parts = []
    pos = 0
    for b,e,f in words: # sorted(words, key=lambda t:t[0]):
        if pos < b:
            parts.append( (pos,b, '.%d'%pos) )
        parts.append( (b,e,f) )
        pos = e
    if pos < total_len:
        parts.append( (pos,total_len,'.$') )
    return parts

class Main(object):
    def __init__(self, *args, c_inc='Korean.h', wav='tts_ko.data.wav', **kvargs):
        self.segment = AudioSegment.from_wav(wav)
        self.words = _parse_c_inc_file(c_inc)
        self.parts = _complete_parts(self.words, len(self.segment))

    def split(self, work_dir='/tmp'):
        if not os.path.exist(work_dir):
            os.makedirs(work_dir)
        for b,e,f in self.parts:
            wavfp = os.path.join(work_dir,f+'.wav')
            with open(wavfp, 'wb') as wavf:
                #if b < e:
                seg = self.segment[b:e]
                print('split:', wavfp, b, e-b, len(seg))
                seg.export(wavf, format='wav')

    def join(self, work_dir, out_fp, out_inc_h):
        out_inc_h = open(out_inc_h, 'w')
        segment = AudioSegment.empty() # segment = AudioSegment()
        for _,_,f in self.parts:
            wavfp = os.path.join(work_dir,f+'.wav')
            if os.path.getsize(wavfp) > 44:
                seg = AudioSegment.from_wav(wavfp)
                sbeg = len(segment)
                slen = len(seg)
                if not f.startswith('.'):
                    s = '    {%d<<POS_SHIFT,%d<<POS_SHIFT},//%s' % (sbeg, sbeg+slen, f)
                    print(s, file=out_inc_h)
                    print(s)
                segment += seg
            #print('join', wavfp, slen)
        segment.export(out_fp, format='wav')

    def help(self, *args, **kvargs):
        print('Usages:'
                '\t{0} c_inc=<Korean.h> wav=<tts_ko.wav> split /tmp'
                '\t{0} c_inc=<Korean.h> wav=<tts_ko.wav> join /tmp /tmp/ts_ko.wav /tmp/Korean.h'
                .format(sys.argv[0]))

    def silence_a(self):
        sound_file = AudioSegment.from_wav("a-z.wav")
        audio_chunks = silence.split_on_silence(sound_file, 
                # must be silent for at least half a second
                min_silence_len=500,

                # consider it silent if quieter than -16 dBFS
                silence_thresh=-16
                )

        for i, chunk in enumerate(audio_chunks):
            out_file = ".//splitAudio//chunk{0}.wav".format(i)
            print( "exporting", out_file )
            chunk.export(out_file, format="wav")

#def main():
#    segment = AudioSegment.from_wav('tts_ko.data.wav')
#    words = _parse_c_inc_file('Korean.h')
#    parts = _complete_parts(words, len(segment))
#    #split(parts, segment, 'out')
#    join(parts, 'out', 'out/tts_ko.data.wav', 'out/tts_ko.data.h')
def main(fn, args, kvargs):
    t0 = time.time()
    m = Main(*args, **kvargs);
    getattr(m, fn, m.help)(*args, **kvargs)
    print('time({}): {}"{}'.format(fn, *map(int, divmod(time.time() - t0, 60))) )

if __name__ == '__main__':
    def _fn_lis_dic(args):
        fn, lis, dic = None, [], {} # defaultdict(list)
        for a in args:
            if a.startswith('-'):
                assert ( '=' in a )
                a = a.strip('-')
            if '=' in a:
                k,v = a.split('=',1)
                v0 = dic.setdefault(k,v)
                if v0 is not v:
                    if type(v0) == list:
                        v0.append(v)
                    else:
                        dic[k] = [v0, v]
            else:
                if fn == None:
                    fn = a
                else:
                    lis.append(a)
        return fn, lis, dic
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    try:
        # main()
        import signal
        signal.signal(signal.SIGINT, _sig_handler)
        main(*_fn_lis_dic(sys.argv[1:]))
    except Exception as e:
        #print(e, file=sys.stderr)
        raise

