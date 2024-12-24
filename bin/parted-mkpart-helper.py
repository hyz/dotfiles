#!/usr/bin/env python3
### http://rainbow.chard.org/2013/01/30/how-to-align-partitions-for-best-performance-using-parted/

import sys, os, math

# Sector size (logical/physical): 512B/4096B
#S_logical, S_physical = 512, 4096

TB, GB, MB = 1024**4, 1024**3, 1024**2

def ceil(a, sec_pos):
    return int((sec_pos + (a-1)) / a) * a
def floor(a, sec_pos):
    return int(sec_pos / a) * a

def main(sdx, begin,end, size_bytes, optimal_io_size, physical_block_size):
    size = abs(size_bytes) / physical_block_size
    optimal_io_size = optimal_io_size / physical_block_size

    #scm = optimal_io_size*physical_block_size/math.gcd(optimal_io_size,logical_block_size)/physical_block_size #smallest_cm
    #print('scm={scm:2},{scmM:2}M'.format(scm=scm, scmM=(scm*512/MB))
    #        , 'M={}s G={}s'.format(int(MB/logical_block_size), int(GB/logical_block_size)))
    #lcms = int(scm)

    #bflag, eflag = opt.begin[-1], opt.end[-1] #begin, bflag = int(opt.begin[:-1]), opt.begin[-1] # end, eflag = int(opt.end[:-1]), opt.end[-1]
    #begin, end = int(opt.begin[:-1]), int(opt.end[:-1])

    if size_bytes < 0:
        b, e = end - size, end
        _, x = divmod(b, optimal_io_size)
        if x < optimal_io_size / 2 and begin <= b - x:
            b -= x
        else:
            b += optimal_io_size - x
        #a = (end - begin) * physical_block_size / GB
        #x = (end - b) * physical_block_size / GB
        #print(f'{a}G| ... ({a}G)|')
    else:
        b, e = begin, begin + size
        _, x = divmod(b, optimal_io_size)
        b += optimal_io_size - x
        #a = (end - begin) * physical_block_size / GB
        #x = (b - begin) * physical_block_size / GB
        #print(f'{a}G|({a}G) ... |')
    assert begin <= b and e <= end and b < e
    assert b % optimal_io_size == 0
    return int(b), int(e)

# quotient, remainder = divmod(end, lcms)
# lcm = optimal_io_size*physical_block_size/math.gcd(optimal_io_size,physical_block_size)/logical_block_size #least_common_multiple

if __name__ == '__main__':
    def options():
        import argparse
        argp = argparse.ArgumentParser()
        argp.add_argument('-d', '--drive', default='/dev/sda')
        argp.add_argument('-o', '--io_sizes', default=None)
        argp.add_argument('-t', '--tail', default=True)
        argp.add_argument('begin', help='range begin _s') #(, nargs=2)
        argp.add_argument('end', help='range end _s') #(, nargs='?')
        argp.add_argument('size', help='size to allocate (M/G)') #(, nargs='?')
        return argp.parse_args()
    try:
        opt = options()
        print(' ', opt.drive)
        sdx = os.path.basename(opt.drive)
        assert opt.begin.endswith('s') and opt.end.endswith('s')
        assert opt.size[-1] in ('M', 'G')
        #bflag, eflag = opt.begin[-1], opt.end[-1]

        size_bytes = int(opt.size[:-1]) * (MB,GB)[opt.size[-1]=='G'] * (1,-1)[opt.tail]
        begin = int(opt.begin[:-1])
        end = int(opt.end[:-1]) + 1
        assert begin < end, f'{begin} < {end}'

        # properly aligned for best performance: 1917873536s % 65535s != 0s ## 65535s*512 ~ 32MB
        if opt.io_sizes:
            optimal_io_size, physical_block_size, logical_block_size, *_ = [int(x.strip()) for x in opt.io_sizes.split(',')] + [512,512]
        else:
            optimal_io_size, physical_block_size, logical_block_size = readints(f'/sys/block/{sdx}/queue/optimal_io_size', f'/sys/block/{sdx}/queue/physical_block_size', f'/sys/block/{sdx}/queue/logical_block_size')

        if optimal_io_size == 0:
            optimal_io_size = physical_block_size*65535
            #if opt.align:
            #    if opt.align[-1] == 'M':
            #        optimal_io_size= int(opt.align[:-1])*MB
            #    else:
            #        optimal_io_size= int(opt.align)
            #print(' ', f'optimal_io_size=0,{optimal_io_size}', f'physical_block_size={physical_block_size}')
        print(' ', f'optimal_io_size={optimal_io_size} physical_block_size={physical_block_size} logical_block_size={logical_block_size}')
        assert physical_block_size % 512== 0
        assert optimal_io_size % physical_block_size == 0
        assert optimal_io_size % logical_block_size == 0
        assert logical_block_size % physical_block_size == 0
        assert MB % physical_block_size == 0 # && GB % optimal_io_size == 0

        begin, end = main(sdx, begin, end, size_bytes, optimal_io_size, physical_block_size)
        end -= 1
        g = (end - begin) * physical_block_size / GB
        print(f'mkpart primary {begin}s {end}s # {g:.3}G')
    except Exception:
        print('Example:')
        print('   ', sys.argv[0], '1260218368s 32G')
        raise

def readints(*fs):
    v = []
    for f in fs:
        with open(f) as f:
            v.append( int(f.read().strip()) )
    return v

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

