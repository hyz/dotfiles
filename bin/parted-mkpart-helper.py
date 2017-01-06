#!/usr/bin/env python3

import sys

# Sector size (logical/physical): 512B/4096B
sec_size_log = 512
sec_size_phy = 4096

def physical_align(a, sec_begin, n_sec):
    sec_begin = int((sec_begin + (a-1)) /a) * a    # aligned START
    sec_end = sec_begin + n_sec
    sec_end = int((sec_end + (a-1)) /a) * a    # aligned START
    return int(sec_begin), int(sec_end)-1

if __name__ == '__main__':
    try:
        assert sec_size_phy % sec_size_log == 0
        assert len(sys.argv) == 3
        _, sec_begin, exp_size = '512B/4096B', sys.argv[1], sys.argv[2]
        #if len(sys.argv) > 3: sec_size_phy, sec_begin, exp_size = sys.argv[1], sys.argv[2], sys.argv[3]
        #assert sec_size_phy[-1] == 'B'
        assert sec_begin[-1] == 's'
        assert exp_size[-1] in ('G','M')
        G, M = 1024*1024*1024, 1024*1024

        #sec_size_phy = int(sec_size_phy.rstrip('B'))
        sec_begin = max(int(sec_begin.rstrip('s')) * sec_size_log, sec_size_phy)
        if exp_size[-1] == 'G':
            exp_size = int(exp_size.rstrip('G'))*G
        else:
            exp_size = int(exp_size.rstrip('M'))*M
        v = physical_align(M/sec_size_phy, sec_begin/sec_size_phy, exp_size/sec_size_phy)
        v = map(lambda x:'%.0fs'%(x*sec_size_phy/sec_size_log), v)
        print('mkpart primary', *v)
    except Exception:
        print('Example:')
        print('\t%s 4096B 1260218368s 32G' % sys.argv[0])
        print()
        raise


