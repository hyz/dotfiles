#!/usr/bin/env python3
### http://rainbow.chard.org/2013/01/30/how-to-align-partitions-for-best-performance-using-parted/

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
    S_count = 65535*4 #int(1000*M / S_logical)
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

#
# (parted) p free                                                           
# Model: Seagate BUP Slim GD (scsi)
# Disk /dev/sdb: 1953525167s
# Sector size (logical/physical): 512B/4096B
# Partition Table: msdos
# Disk Flags: 
# 
# Number  Start        End          Size         Type     File system  Flags
#         63s          2047s        1985s                 Free Space
#  1      2048s        1953521663s  1953519616s  primary  ntfs
#         1953521664s  1953525166s  3503s                 Free Space
# 
# wood@arch> cat /sys/block/sdb/queue/optimal_io_size                                                                      ~
# 33553920
# wood@arch> cat /sys/block/sdb/queue/minimum_io_size                                                                          ~
# 4096
# wood@arch> cat /sys/block/sdb/alignment_offset                                                                               ~
# 0
# wood@arch> cat /sys/block/sdb/queue/physical_block_size                                                                      ~
# 4096
# wood@arch> sudo parted /dev/sdb align-check optimal 1
# 1 not aligned
# wood@arch> python                                                                                                            ~
# Python 3.6.1 (default, Mar 27 2017, 00:27:06) 
# [GCC 6.3.1 20170306] on linux
# Type "help", "copyright", "credits" or "license" for more information.
# >>> 33553920/4096
# 8191.875
# >>> 33553920/1024
# 32767.5
# >>> 33553920/512
# 65535.0
# >>> 

