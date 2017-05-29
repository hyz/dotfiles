#!/usr/bin/env python3

import sys, re

# Sector size (logical/physical): 512B/4096B
#S_logical, S_physical = 512, 4096

def ceil(a, sec_pos):
    return int((sec_pos + (a-1)) / a) * a
def floor(a, sec_pos):
    return int(sec_pos / a) * a

def args():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--sector", default="512B/4096B")
    parser.add_argument("begin")
    parser.add_argument("end")
    args = parser.parse_args()
    assert args.begin.endswith('s')
    assert args.end[-1] in ('s', 'M', 'G')

    G, M = 1024*1024*1024, 1024*1024
    res = re.match('(\d+)B/(\d+)B', args.sector)
    S_logical, S_physical = int(res.group(1)), int(res.group(2))
    assert S_physical % S_logical == 0
    assert M % S_physical == 0
    S_count = int(M / S_logical)
    print('\t\t##', f'Sector size (logical/physical): {S_logical}B/{S_physical}B')

    assert args.begin[-1] == 's' and args.end[-1] in ('s', 'M', 'G')
    args.begin = int(args.begin[:-1]) #max(int(args.begin[:-1]), S_count)
    args.begin = ceil(S_count, args.begin)

    args.end, tag = int(args.end[:-1]), args.end[-1]
    if tag != 's':
        args.end = args.begin + (args.end * (M,G)[tag=='G']) / S_logical
    args.end = floor(S_count, args.end)-1

    #print( args.begin, args.end, args.sector )
    assert args.begin < args.end
    return args.begin, int(args.end)

if __name__ == '__main__':
    try:
        sec_begin, sec_end = args()
        print(f'mkpart primary {sec_begin}s {sec_end}s')
    except Exception:
        print('Example:')
        print('   ', sys.argv[0], '-s 512B/4096B 1260218368s 32G')
        raise

