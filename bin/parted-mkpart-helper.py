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

def main(sdx, begin,end, bflag,eflag):
    def readints(*fs):
        v = []
        for f in fs:
            with open(f) as f:
                v.append( int(f.read().strip()) )
        return v
    optimal_io_size, physical_block_size, logical_block_size = readints(f'/sys/block/{sdx}/queue/optimal_io_size', f'/sys/block/{sdx}/queue/physical_block_size', f'/sys/block/{sdx}/queue/logical_block_size')
    if optimal_io_size == 0:
        optimal_io_size = max(4096, physical_block_size)
        if opt.align:
            if opt.align[-1] == 'M':
                optimal_io_size= int(opt.align[:-1])*MB
            else:
                optimal_io_size= int(opt.align)
        print(' ', f'optimal_io_size=0,{optimal_io_size}', f'physical_block_size={physical_block_size}')
    print(' ', f'optimal_io_size={optimal_io_size} physical_block_size={physical_block_size} logical_block_size={logical_block_size}')
    assert optimal_io_size % logical_block_size == 0
    assert physical_block_size % logical_block_size == 0
    assert MB % logical_block_size == 0

    scm = optimal_io_size*physical_block_size/math.gcd(optimal_io_size,physical_block_size)/logical_block_size #smallest_cm
    print('scm={scm:2},{scmM:2}M'.format(scm=scm, scmM=(scm*512/MB))
            , 'M={}s G={}s'.format(int(MB/logical_block_size), int(GB/logical_block_size)))
    lcms = int(scm)

    bflag, eflag = opt.begin[-1], opt.end[-1] #begin, bflag = int(opt.begin[:-1]), opt.begin[-1] # end, eflag = int(opt.end[:-1]), opt.end[-1]
    begin, end = int(opt.begin[:-1]), int(opt.end[:-1])
    assert bflag in ('s', 'M', 'G')
    assert eflag in ('s', 'M', 'G')
    assert 's' in (bflag, eflag)

    if bflag == 's':
        begin = ceil(lcms, begin)
        if eflag != 's':
            size = end
            size = int((size * (MB,GB)[eflag=='G']) / logical_block_size)
            end = begin + size
        end = floor(lcms, end)
    else:
        end = floor(lcms, end)
        if bflag != 's':
            size = begin
            size = int((size * (MB,GB)[bflag=='G']) / logical_block_size)
            begin = end - size
        begin = ceil(lcms, begin)
    end -= 1

    #print( begin, end, opt.sector )
    assert begin < end
    return begin, end

if __name__ == '__main__':
    def options():
        import argparse
        argp = argparse.ArgumentParser()
        argp.add_argument('-d', '--drive', default='/dev/sda')
        argp.add_argument('-a', '--align', default=None)
        argp.add_argument('begin', help='begin/size') #(, nargs=2)
        argp.add_argument('end', help='end/size') #(, nargs='?')
        return argp.parse_args()
    try:
        opt = options()
        print(' ', opt.drive)
        sdx = os.path.basename(opt.drive)
        sec_begin, sec_end = main(sdx, int(opt.begin[:-1]), int(opt.end[:-1]), opt.begin[-1], opt.end[-1])
        print(f'mkpart primary {sec_begin}s {sec_end}s')
    except Exception:
        print('Example:')
        print('   ', sys.argv[0], '1260218368s 32G')
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

