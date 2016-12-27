#!/usr/bin/env python3

import sys, os, re
import subprocess, shutil, glob, tempfile
from pprint import pprint

_defs = {
    'REPO' : 'NewGame'
  , 'VARIANT' : 'release'
}

HOME=os.environ['HOME']
_BuildDir=os.path.join(HOME,'build')
_AppConfig='src/com/huazhen/barcode/app/AppConfig.java'

def prepare(*a, VARIANT='release', DESTDIR=_BuildDir, **d):
    def ximpl(src, variant, out):
        global_vars_init(src)
        out, prjname = os.path.abspath(out) , os.path.basename(src)
        #print(OldVer,NewVer, OldSVNRev, NewSVNRev)
        #print(prjname, src, out, a, d)
        pprint(locals())
        pprint(globals())

        if not os.path.exists(src):
            die(src)
        if not out.startswith( _BuildDir ):
            die(out, tail)
        out = os.path.join(out, prjname)

        if not os.path.exists(out):
            os.makedirs(out, exist_ok=True)
        shutil.rmtree(out, ignore_errors=True )

        ignore = lambda d,names: [ '.svn','.git' ]
        shutil.copytree(src, out, ignore=ignore)
        etree_replace_text(os.path.join(out,'.project'), './name', prjname)

        if variant == 'release':
            shutil.copyfile(os.path.join(src,'../tools/CryptoRelease.bat'), os.path.join(out,'tools/Crypto.bat'))
            log_h = os.path.join(out,'jni/Utils/log.h')
            ed(log_h, '^[^\s]+#\s*define\s+BUILD_RELEASE', '#define BUILD_RELEASE', 1)
            # sed -i '/^[^#]\+#\s*define\s\+BUILD_RELEASE/{s:^[^#]\+::}' $builddir/$prjname/jni/Utils/log.h
        version_set(os.path.join(out,_AppConfig), NewVer, NewSVNRev, variant)

    ximpl(os.path.join(HOME,'release/BarcodeAndroid'), VARIANT, DESTDIR)
    global OldSVNRev, NewSVNRev, OldVer, NewVer
    OldSVNRev = NewSVNRev = None
    ximpl(os.path.join(HOME,'release/BarcodeAndroidNewUI'), VARIANT, DESTDIR)

def version_set(appconfig, ver, svnrev, variant):
    lines = []
    with open(appconfig) as sf:
        lines = sf.readlines()

    re_ver = re.compile('\spublic\s.*\sString\s+VERSION\s*=\s*"v\d+\.\d+\.\d+(-r\d+)?"')
    re_svnrev = re.compile('\spublic\s.*\sString\s+SVNVERSION\s*=\s*"new-svn\d+"')
    with open(appconfig, 'w') as outf:
        def repf(r):
            if 'SVNVERSION' in r.group(0):
                return re.sub('=\s*"new-svn\d+"', '= "new-svn{}"'.format(svnrev), r.group(0))
            if variant == 'release':
                return re.sub('=\s*"v[^"]+"', '= "v{}.{}.{}"'.format(*ver), r.group(0))
            return re.sub('=\s*"v[^"]+"', '= "v{}.{}.{}-{}"'.format(ver[0],ver[1],ver[2], svnrev), r.group(0))
        for line in lines:
            lin = re.sub(re_ver, repf, line)
            if lin is line:
                lin = re.sub(re_svnrev, repf, line)
            outf.write(lin)

def global_vars_init(src):
    global OldSVNRev, NewSVNRev, OldVer, NewVer
    OldSVNRev = os.environ.get('OldSVNRev', None)
    NewSVNRev = os.environ.get('NewSVNRev', None)
    NewVer = os.environ.get('NewVer', None)
    OldVer = os.environ.get('OldVer', None)

    re_ver = re.compile('\spublic\s.*\sString\s+VERSION\s*=\s*"v(\d+)\.(\d+)\.(\d+)(-r\d+)?"')
    re_svnrev = re.compile('\spublic\s.*\sString\s+SVNVERSION\s*=\s*"new-svn(\d+)"')
    src = os.path.join(src,_AppConfig)
    with open(src) as sf:
        for line in sf:
            if not OldVer:
                r = re.search(re_ver, line)
                if r:
                    OldVer = r.group(1,2,3)
            if not OldSVNRev:
                r = re.search(re_svnrev, line)
                if r:
                    OldSVNRev = r.group(1)
    if not NewVer:
        NewVer = ( OldVer[0], OldVer[1], str(int(OldVer[2])+1) )
    if not NewSVNRev:
        #print('svn info "{}" |grep -Po "^Revision:\s+\K\d+"'.format(src))
        NewSVNRev = subprocess.check_output('svn info "{}" |grep -Po "^Revision:\s+\K\d+"'.format(src), shell=True)
        NewSVNRev = NewSVNRev.decode().strip()

def etree_replace_text(filename, path, text):
    from xml.etree.ElementTree import ElementTree
    tree = ElementTree(file=filename)
    node = tree.find(path)
    if node != None:
        node.text = text
        tree.write(filename, encoding='UTF-8', xml_declaration=True) #(, short_empty_elements=False)

def ed(textf, expr, ss, nrep=-1):
    lines = []
    with open(textf) as sf:
        lines = sf.readlines()
    with open(textf, 'w') as outf:
        for line in lines:
            if nrep > 0:
                l, line = line, re.sub(expr, ss, line)
                if not (l is line):
                    nrep -= 1
            outf.write( line )

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
    def _help(*a, **d):
        print(sys.argv[0], 'prepare a b c a=1 b=2 a=3')
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1

    try:
        import signal
        signal.signal(signal.SIGINT, _sig_handler)
        lis, dic = _args_lis_dic(sys.argv[2:], _defs)
        globals().get(sys.argv[1], _help)(*lis, **dic)
    except Exception as e:
        print(e, file=sys.stderr)
        raise

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

