#!/usr/bin/env python3
import sys

def main(sec_size, sec_begin, exp_size):
    a = M/sec_size
    sec_begin = int((sec_begin + (a-1)) /a) * a    # aligned START
    sec_end = sec_begin + exp_size/sec_size
    sec_end = int((sec_end + (a-1)) /a) * a    # aligned START
    return sec_begin, sec_end-1

if __name__ == '__main__':
    try:
        sec_size, sec_begin, exp_size = '4096B', sys.argv[1], sys.argv[2]
        if len(sys.argv) > 3:
            sec_size, sec_begin, exp_size = sys.argv[1], sys.argv[2], sys.argv[3]
        assert sec_size[-1] == 'B'
        assert sec_begin[-1] == 's'
        assert exp_size[-1] in ('G','M')

        G, M = 1024*1024*1024, 1024*1024
        if exp_size[-1] == 'G':
            exp_size = int(exp_size.rstrip('G'))*G
        else:
            exp_size = int(exp_size.rstrip('M'))*M

        sec_begin = int(sec_begin.rstrip('s'))
        sec_size = int(sec_size.rstrip('B'))

        print('mkpart primary', *map(lambda x:x+'s', map(str,map(int,main(sec_size, sec_begin, exp_size)))) )
    except Exception:
        print('Example:')
        print('\talign-parted.py 4096B 1260218368s 32G')
        print()
        raise


