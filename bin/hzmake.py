#!/usr/bin/env python3

import sys, time, os, re
import subprocess, shutil, glob, tempfile
from pprint import pprint

HOME = os.environ['HOME']
# setattr(sys.modules[__name__], x, os.environ.get(x, None))

class Project(object):
    _AppConfig='src/com/huazhen/barcode/app/AppConfig.java'

    def __init__(self, src, *a, **d):
        for x in 'OldSVNRev', 'NewSVNRev', 'OldVer', 'NewVer':
            setattr(self, x, d.get(x, os.environ.get(x, getattr(self,x,None))))
        self.name = os.path.basename(src)

        re_ver = re.compile('\spublic\s.*\sString\s+VERSION\s*=\s*"v(\d+)\.(\d+)\.(\d+)(-r\d+)?"')
        re_svnrev = re.compile('\spublic\s.*\sString\s+SVNVERSION\s*=\s*"new-svn(\d+)"')
        src = os.path.join(src,self._AppConfig)
        with open(src) as sf:
            for line in sf:
                if not self.OldVer:
                    r = re.search(re_ver, line)
                    if r:
                        self.OldVer = r.group(1,2,3)
                if not self.OldSVNRev:
                    r = re.search(re_svnrev, line)
                    if r:
                        self.OldSVNRev = r.group(1)
        if not self.NewVer:
            self.NewVer = ( self.OldVer[0], self.OldVer[1], str(int(self.OldVer[2])+1) )
        if not self.NewSVNRev:
            #print('svn info "{}" |grep -Po "^Revision:\s+\K\d+"'.format(src))
            self.NewSVNRev = subprocess.check_output('svn info "{}" |grep -Po "^Revision:\s+\K\d+"'.format(src), shell=True)
            self.NewSVNRev = self.NewSVNRev.decode().strip()

class Main(object):
    PLATS   = ['g500','k400','cvk350c','cvk350t']
    VARIANT = 'release'
    SRCDIR  = os.path.join(HOME,'release')
    DESTDIR = os.path.join(HOME,'build')

    _AppConfig='src/com/huazhen/barcode/app/AppConfig.java'
    _PROJECTS = {
            'BarcodeAndroid': [ 'k400', 'cvk350c', 'cvk350t' ]
            , 'BarcodeAndroidNewUI': [ 'g500' ]
        }

    @staticmethod
    def run(func, a, d):
        m = Main(*a, **d);
        getattr(m, func, m.help)(*a, **d)

    def __init__(self, *a, **d):
        #self._ver = Project()
        for x in 'DESTDIR', 'SRCDIR' , 'VARIANT', 'PLATS' :
            setattr(self, x, d.get(x, os.environ.get(x, getattr(self,x,None))))
        if type(self.PLATS) == str:
            self.PLATS = self.PLATS.split(',')
        for x in self._PROJECTS.keys():
            setattr(self, x, Project( os.path.join(self.SRCDIR,x), *a, **d ))

    def prepare(self, *a, **d):
        def impl(self, prj):
            src = os.path.abspath( os.path.join(self.SRCDIR ,prj.name) ) #, os.path.basename(src)
            out = os.path.abspath( os.path.join(self.DESTDIR,prj.name) ) #, os.path.basename(src)

            if not os.path.exists(src):
                die(src, 'Not exists')
            if not out.startswith( Main.DESTDIR ):
                die(out, 'should startswith', Main.DESTDIR)
            pprint(locals())

            if not os.path.exists(out):
                os.makedirs(out, exist_ok=True)
            shutil.rmtree(out, ignore_errors=True )

            ignore = lambda d,names: [ '.svn','.git' ]
            shutil.copytree(src, out, ignore=ignore)
            etree_replace_text(os.path.join(out,'.project'), './name', prj.name)

            if self.VARIANT == 'release':
                shutil.copyfile(os.path.join(src,'../tools/CryptoRelease.bat'), os.path.join(out,'tools/Crypto.bat'))
                log_h = os.path.join(out,'jni/Utils/log.h')
                ed(log_h, '^[^\s]+#\s*define\s+BUILD_RELEASE', '#define BUILD_RELEASE', 1)

            if prj.name == 'BarcodeAndroidNewUI':
                ed(os.path.join(out,'AndroidManifest.xml')
                        , '\sandroid:versionName="(\d+\.\d+)"\s'
                        , ' android:versionName="\\1" android:sharedUserId="android.uid.system" ')

            self.version_set(os.path.join(out,self._AppConfig), prj.NewVer, prj.NewSVNRev)

            for fp in glob.glob(os.path.join(out, 'doc/*.xls')):
                shutil.copy(fp, '/samba/release1/doc/')

        prjs = {}
        for plat in self.PLATS:
            for prjname,plats in self._PROJECTS.items():
                if plat in plats:
                    prjs[prjname] = getattr(self,prjname) # prjs.setdefault(prjname,[]).append(plat)
        for prj in prjs.values():
            impl(self, prj)

    def version_set(self, appconfig, ver, svnrev):
        lines = []
        with open(appconfig) as sf:
            lines = sf.readlines()

        re_ver = re.compile('\spublic\s.*\sString\s+VERSION\s*=\s*"v\d+\.\d+\.\d+(-r\d+)?"')
        re_svnrev = re.compile('\spublic\s.*\sString\s+SVNVERSION\s*=\s*"new-svn\d+"')
        with open(appconfig, 'w') as outf:
            def repf(r):
                if 'SVNVERSION' in r.group(0):
                    return re.sub('=\s*"new-svn\d+"', '= "new-svn{}"'.format(svnrev), r.group(0))
                if self.VARIANT == 'release':
                    return re.sub('=\s*"v[^"]+"', '= "v{}.{}.{}"'.format(*ver), r.group(0))
                return re.sub('=\s*"v[^"]+"', '= "v{}.{}.{}-{}"'.format(ver[0],ver[1],ver[2], svnrev), r.group(0))
            for line in lines:
                lin = re.sub(re_ver, repf, line)
                if lin is line:
                    lin = re.sub(re_svnrev, repf, line)
                outf.write(lin)

    def version(prj):
        pass

    def make(self, *a, **d):
        if self.VARIANT != 'release':
            return

        if 'g500' in self.PLATS:
            for fp in 'libs/armeabi-v7a/libBarcode.so', 'libmtkhw.so', 'release/':
                #mt6580/alps/packages/apps/Game
                pass
            cmd='cd mt6580/alps && source build/envsetup.sh && lunch full_ckt6580_we_l-user && make -j8'
            subprocess.Popen(cmd, shell=True, executable='/bin/bash')
            #cmd='autocopy'
            #subprocess.Popen(cmd, shell=True, executable='/bin/bash')

        cmd='make -f BarcodeAndroid.mk PLATS=k400,cvk350c,cvk350t release RARPWD=huaguanjiye'
        subprocess.Popen(cmd, shell=True, executable='/bin/bash')

    def version_commit(self, *a, **d):
        pass

    def help(self, *a, **d):
        print('''\
Usages:
    {0} prepare PLATS={sPLATS} VARIANT=[release|test]
    {0} make PLATS={sPLATS} VARIANT=[release|test] RARPWD=XXX
    {0} version_commit
'''.format(sys.argv[0], sPLATS=','.join(Main.PLATS), **vars(self)))

        self.info(*a, **d)
        print()
        ver = lambda v: '.'.join( str(x) for x in v )
        for k in self._PROJECTS.keys():
            prj = getattr(self, k, None)
            if prj:
                print('apk: {}/{}-{}-r{}-{}.apk'.format(self.VARIANT, prj.name, ver(prj.NewVer), prj.NewSVNRev, time.strftime('%Y%m%d')))
            #pprint(vars(self)) #pprint(globals())
        grep('word', 'howto.txt')

    def info(self, *a, **d):
        pprint(vars(self))
        if a: pprint(a)
        if d: pprint(d)

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

def grep(expr, *files, filt=None):
    try:
        for textf in files:
            with open(textf) as sf:
                for line in sf:
                    res = re.search(expr, line)
                    if res:
                        if filt: line = filt(line, res)
                        if line:
                            print(line.strip())
    except IOError as e:
        print(textf, e)

def die(*a):
    print(*a, file=sys.stderr)
    sys.exit(127)

def _args_lis_dic(args):
    lis, dic = [], {} # defaultdict(list)
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
    #for k,v in defs.items(): dic.setdefault(k, v)
    return lis, dic

if __name__ == '__main__':
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    try:
        import signal
        signal.signal(signal.SIGINT, _sig_handler)
        Main.run(sys.argv[1], *_args_lis_dic(sys.argv[2:]))
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

