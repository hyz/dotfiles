#!/usr/bin/env python3

import sys, re

# Sector size (logical/physical): 512B/4096B
#sec_size_log, sec_size_phy = 512, 4096

def align(a, sec_begin, sec_count):
    sec_begin = int((sec_begin + (a-1)) /a) * a    # aligned START
    sec_end = sec_begin + sec_count
    sec_end = int((sec_end + (a-1)) /a) * a    # aligned START
    return sec_begin, sec_end - 1

if __name__ == '__main__':
    G, M = 1024*1024*1024, 1024*1024

    try:
        assert len(sys.argv) >= 3
        sec_begin, exp_size, s_log_phy = sys.argv[1], sys.argv[2], '512B/4096B'
        assert sec_begin[-1] in ('s',)
        assert exp_size[-1] in ('G','M')
        if len(sys.argv) == 4:
            s_log_phy = sys.argv[3]
        res = re.match('(\d+)B/(\d+)B', s_log_phy)
        assert res
        sec_size_log, sec_size_phy = int(res.group(1)), int(res.group(2))
        assert sec_size_phy % sec_size_log == 0
        assert M % sec_size_phy == 0

        sec_count = int(exp_size.rstrip('MG')) * (M,G)[exp_size[-1] == 'G'] / sec_size_log
        sec_begin = max(int(sec_begin.rstrip('s')), int(M / sec_size_log))
        sec_begin, sec_count = align(int(M/sec_size_log), sec_begin, sec_count)

        print(f'mkpart primary {sec_begin}s {sec_count}s')
        print('\t###', f'Sector size (logical/physical): {sec_size_log}B/{sec_size_phy}B')
    except Exception:
        print('Example:')
        print('   ', sys.argv[0], '1260218368s 32G 512B/4096B')
        raise

