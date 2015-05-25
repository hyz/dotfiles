import sys, os
import getopt

bstok = '0c4707dab8c186a7bcaffa58abeb2310'

def gen_user_reg_json():
    fr_numb = 49990
    count = 10
    ofp = None

    if len(sys.argv) > 1:
        opts, args = getopt.getopt(sys.argv[1:],"ho:")
        opts = dict(opts)
        if '-h' in opts or '--help' in opts:
            print 'Usage: {0} -o <outs> <from-uid> <count>'.format(sys.argv[0])
            sys.exit(0)
        for o,v in opts.items():
            ofp = v
        if len(args):
            if len(args) != 2:
                sys.exit(1)
            fr_numb = int(args[0])
            count = int(args[1])
    
    f_ids = f_reg = f_sig = sys.stdout
    if ofp:
        f_ids = open(ofp + '.user', 'w')
        f_reg = open(ofp + '.reg', 'w')
        f_sig = open(ofp + '.sign', 'w')

    fmt_reg = '{0}\t111\t{{"version":"1.0.0","appid":"yx","cmd":111,"auth":"{1}","body":{{"uid":{0},"token":"={0}="}}}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_reg.write( fmt_reg.format(x, bstok) );

    fmt_sig = '{0}\t99\t{{"version":"1.0.0","cmd":99,"body":{{"appid":"yx","uid":{0},"token":"={0}="}}}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_sig.write( fmt_sig.format(x) );

    fmt_ids = '{0}\t1\t{{0:0}}\n'
    for x in range(fr_numb, fr_numb + count):
        f_ids.write( fmt_ids.format(x) )

if __name__ == '__main__':
    gen_user_reg_json()


