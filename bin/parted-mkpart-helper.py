#!/usr/bin/env python3

import sys

# Sector size (logical/physical): 512B/4096B
sec_size_log = 512
sec_size_phy = 4096

def align(a, sec_begin, n_secs):
    sec_begin = int((sec_begin + (a-1)) /a) * a    # aligned START
    sec_end = sec_begin + n_secs
    sec_end = int((sec_end + (a-1)) /a) * a    # aligned START
    return int(sec_begin), int(sec_end)-1

if __name__ == '__main__':
    G, M = 1024*1024*1024, 1024*1024
    try:
        assert M % sec_size_phy == 0
        assert sec_size_phy % sec_size_log == 0
        assert len(sys.argv) == 3

        _, sec_begin, exp_size = '512B/4096B', sys.argv[1], sys.argv[2]
        assert sec_begin[-1] in ('s',)
        assert exp_size[-1] in ('G','M')

        begin = max(int(sec_begin.rstrip('s')) * sec_size_log, M)
        if exp_size[-1] == 'G':
            exp_size = int(exp_size.rstrip('G'))*G
        else:
            exp_size = int(exp_size.rstrip('M'))*M
        v = align(M/sec_size_log, begin/sec_size_log, exp_size/sec_size_log)
        v = map(lambda x:'%.0fs'%x, v)
        print('mkpart primary', *v)
    except Exception:
        print('Example:')
        print('\t%s 512B/4096B 1260218368s 32G' % sys.argv[0])
        print()
        raise

