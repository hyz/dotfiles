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
    argp = argparse.ArgumentParser()
    argp.add_argument('-s', '--sector', default='512B/4096B')
    argp.add_argument('begin') #(, nargs=2)
    argp.add_argument('size', help='size/end') #(, nargs='?')
    opt = argp.parse_args()
    assert opt.begin.endswith('s')
    assert opt.size[-1] in ('s', 'M', 'G')

    T, G, M = 1024**4, 1024**3, 1024**2
    res = re.match('(\d+)B/(\d+)B', opt.sector)
    S_logical, S_physical = int(res.group(1)), int(res.group(2))
    assert S_physical % S_logical == 0
    assert M % S_physical == 0
    S_count = int(M / S_logical)
    print('\t\t##', f'Sector size (logical/physical): {S_logical}B/{S_physical}B')

    assert opt.begin[-1] == 's' and opt.size[-1] in ('s', 'M', 'G')
    begin = int(opt.begin[:-1]) #max(int(opt.begin[:-1]), S_count)
    begin = ceil(S_count, begin)

    size, tag = int(opt.size[:-1]), opt.size[-1]
    if tag == 's':
        end = size
    else:
        end = begin + (size * (M,G)[tag=='G']) / S_logical
    end = floor(S_count, end)-1

    #print( begin, end, opt.sector )
    assert begin < end
    return begin, end

if __name__ == '__main__':
    try:
        sec_begin, sec_end = args()
        print(f'mkpart primary {sec_begin}s {sec_end}s')
    except Exception:
        print('Example:')
        print('   ', sys.argv[0], '-s 512B/4096B 1260218368s 32G')
        raise

