#!/usr/bin/env python3

from pydub import AudioSegment, silence
import os, re

def silence_a():
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

def get_pcm_words(infp):
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

def complete_parts(words, total_len=0):
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

def split(parts, segment, outdir):
    for b,e,f in parts:
        wavfp = os.path.join(outdir,f+'.wav')
        with open(wavfp, 'wb') as wavf:
            #if b < e:
            seg = segment[b:e]
            seg.export(wavf, format='wav')
            print('split', wavfp, b, e-b, len(seg))

def join(parts, indir, outfp, inc_h):
    inc_h = open(inc_h, 'w')
    segment = None
    for _,_,f in parts:
        wavfp = os.path.join(indir,f+'.wav')
        millis = 0
        if os.path.getsize(wavfp) > 44:
            seg = AudioSegment.from_wav(wavfp)
            millis = len(seg)
            if segment:
                b = len(segment)
                segment += seg
            else:
                b = 0
                segment = seg
            if not f.startswith('.'):
                s = '    {%d<<POS_SHIFT,%d<<POS_SHIFT},//%s' % (b, b+millis, f)
                print(s, file=inc_h)
                #print(s)
        #print('join', wavfp, millis)
    segment.export(outfp, format='wav')

def main():
    segment = AudioSegment.from_wav('tts_ko.data.wav')
    words = get_pcm_words('Korean.h')
    parts = complete_parts(words, len(segment))
    #split(parts, segment, 'out')
    join(parts, 'out', 'out/tts_ko.data.wav', 'out/tts_ko.data.h')

if __name__ == '__main__':
    main()

