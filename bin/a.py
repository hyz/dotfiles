#!/usr/bin/env python3

import sys, os
import subprocess, shutil, glob, tempfile

_defs = {
    'REPO' : 'NewGame'
  , 'VARIANT' : 'release'
}

HOME=os.environ['HOME']
build_dir=os.path.join(HOME,'build')

def prepare(src, *a, out=build_dir, **d):
    out, prjname = os.path.abspath(out) , os.path.basename(src)
    print(prjname, src, out, a, d)

    if not os.path.exists(src):
        die(src)
    if not out.startswith( build_dir ):
        die(out, tail)
    out=os.path.join(out, prjname)

    ignore = lambda d,names: [ '.svn','.git' ]
    if not os.path.exists(out):
        os.makedirs(out, exist_ok=True)
    shutil.rmtree( out, ignore_errors=True )
    shutil.copytree(src, out, ignore=ignore)

    etree_replace_text(os.path.join(out,'.project'), './name', prjname)

def etree_replace_text(filename, path, text):
    from xml.etree.ElementTree import ElementTree
    tree = ElementTree(file=filename)
    node = tree.find(path)
    if node != None:
        node.text = text
        tree.write(filename, encoding='UTF-8', xml_declaration=True) #(, short_empty_elements=False)

def die(*a):
    print(*a, file=sys.stderr)
    sys.exit(127)

def _args_lis_dic(args, defs={}):
    lis, dic = [],  {} # defaultdict(list)
    for a in args:
        if a.startswith('-'):
            assert ( '=' in a )
        a = a.strip('-')
        if '=' in a:
            k,v = a.split('=',1)
            v0 = dic.setdefault(k,v)
            if v0 is not v:
                if type(v0) == list:
                    v0.append(v)
                else:
                    dic[k] = [v0, v]
        else:
            lis.append(a)
    for k,v in defs.items():
        dic.setdefault(k, v)
    return lis, dic

if __name__ == '__main__':
    import signal
    def _help(*a, **d):
        print(sys.argv[0], 'prepare a b c a=1 b=2 a=3')
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    try:
        signal.signal(signal.SIGINT, _sig_handler)
        lis, dic = _args_lis_dic(sys.argv[2:], _defs)
        globals().get(sys.argv[1], _help)(*lis, **dic)
    except Exception as e:
        print(e, file=sys.stderr)

    #import argparse

    #parser = argparse.ArgumentParser()
    #subparsers = parser.add_subparsers()

    #hello_parser = subparsers.add_parser('hello')
    #hello_parser.add_argument('name')
    ## add greeting option w/ default
    #hello_parser.add_argument('--greeting', default='Hello')
    ## add a flag (default=False)
    #hello_parser.add_argument('--caps', action='store_true')
    #hello_parser.set_defaults(func=greet)

    #goodbye_parser = subparsers.add_parser('goodbye')

