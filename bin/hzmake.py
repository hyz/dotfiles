#!/usr/bin/env python3

import sys, time, os, re, types, contextlib
import subprocess, shutil, glob, tempfile
from pprint import pprint

HOME = os.environ['HOME']
_rhost = '192.168.2.113'
# setattr(sys.modules[__name__], x, os.environ.get(x, None))

class Project(object):
    _AppConfig='src/com/huazhen/barcode/app/AppConfig.java'

    def __init__(self, repo, plts, *args, **kvargs):
        if not os.path.exists(repo):
            die(repo, 'Not exists')
        if _FUNC in ('prepare','init'):
            subprocess.check_call(command('cd {} && svn up', repo), shell=True, executable='/bin/bash')

        self.repo = repo;
        self.name = os.path.basename(repo)
        self.plats = plts
        self.datestr = time.strftime('%Y%m%d')

        re_ver = re.compile('\spublic\s.*\sString\s+VERSION\s*=\s*"v(\d+)\.(\d+)\.(\d+)(-r\d+)?"')
        re_svnrev = re.compile('\spublic\s.*\sString\s+SVNVERSION\s*=\s*"new-svn(\d+)"')
        with open(os.path.join(self.repo,self._AppConfig)) as cf:
            for line in cf:
                if not self.OldVer:
                    r = re.search(re_ver, line)
                    if r:
                        self.OldVer = tuple(map(int,r.group(1,2,3)))
                if not self.OldSVNRev:
                    r = re.search(re_svnrev, line)
                    if r:
                        self.OldSVNRev = (int(r.group(1)),)
        if not self.NewVer:
            self.NewVer = ( self.OldVer[0], self.OldVer[1], self.OldVer[2]+1 )
        if not self.NewSVNRev:
            self.NewSVNRev = subprocess.check_output('svn info "{}" |grep -Po "^Revision:\s+\K\d+"'.format(self.repo), shell=True)
            self.NewSVNRev = self.NewSVNRev.decode().strip()
            self.NewSVNRev = (int(self.NewSVNRev),)

    def ver(self, old=0):
        if old:
            return '.'.join(map(str,self.OldVer))
        return '.'.join(map(str,self.NewVer))
    def svnrev(self, old=0):
        if old:
            return '.'.join(map(str,self.OldSVNRev))
        return '.'.join(map(str,self.NewSVNRev))
    def fullver(self):
        return '{}-{}-r{}-{}'.format(self.name, self.ver(), self.svnrev(), self.datestr)
    def platver(prj, plt):
        return '{}-{}-{}'.format(plt, prj.ver(), prj.datestr)
    def vars(self, **kvargs):
        d = vars(self).copy()
        d.update(OldSVNRev=self.svnrev(old=1)
                , NewSVNRev=self.svnrev(), OldVer=self.ver(old=1), NewVer=self.ver())
        d.update(**kvargs)
        return d

    #def __str__(self): return ','.join((OldVer, OldSVNRev, NewVer, NewSVNRev))

class Main(object):
    PLATS   = ['g500','k400','cvk350c','cvk350t']
    VARIANT = 'release'
    REPO  = os.path.join(HOME,'release')
    BUILD_DIR = os.path.join(HOME,'build')

    _PROJECTS = {
        'Game14': [ 'k400', 'cvk350c', 'cvk350t' ]
      , 'Game16': [ 'g500' ]
    }
    _Log_h              = 'jni/Utils/log.h'
    _AndroidManifest    = 'AndroidManifest.xml'
    #_AppConfig          = 'src/com/huazhen/barcode/app/AppConfig.java'
    _Crypto0, _Crypto1  = '../tools/CryptoRelease.bat', 'tools/Crypto.bat'

    def __init__(self, *args, **kvargs):
        for x,y in vars(Main).items(): #'BUILD_DIR', 'REPO' , 'VARIANT', 'PLATS' :
            if not (x.startswith('_') or callable(y)):
                setattr(self, x, kvargs.get(x, os.environ.get(x, getattr(self,x,None))))
                #print('===',x,y)
        if self.VARIANT not in ('release','test'):
            die(self.VARIANT)
        if type(self.PLATS) == str: # and ',' in self.PLATS:
            self.PLATS = self.PLATS.split(',')
        prjs = {}
        for plt in self.PLATS:
            for prjname,plats in self._PROJECTS.items():
                if plt in plats:
                    prjs.setdefault(prjname,[]).append(plt)
                    break
            else:
                die(plt, 'unknown')
        for x in 'OldSVNRev', 'NewSVNRev', 'OldVer', 'NewVer':
            v = kvargs.get(x, os.environ.get(x, getattr(Project,x,None)))
            if v and type(v) == str:
                v = tuple(map(int,v.split('.')))
            setattr(Project, x, v)
        path = lambda prjname: os.path.join(self.REPO,prjname)
        self.projects = [ Project(path(prjname), plts, *args, **kvargs)
                                            for prjname,plts in prjs.items() ]
        ver = max( prj.NewVer for prj in self.projects )
        for prj in self.projects:
            prj.NewVer = ver

    def prepare(self, *args, **kvargs):
        def impl(self, prj):
            src = prj.repo #os.path.abspath( os.path.join(     self.REPO, prj.name) )
            out = os.path.abspath( os.path.join(self.BUILD_DIR, prj.name) )
            print('>>> prepare:', src, out)

            if not os.path.exists(src):
                die(src, 'Not exists')
            if not out.startswith( Main.BUILD_DIR ):
                die(out, 'should startswith', Main.BUILD_DIR)

            if not os.path.exists(out):
                os.makedirs(out, exist_ok=True)
            print('rmtree:', out)
            shutil.rmtree(out, ignore_errors=True )

            print('copytree:', src, out)
            ignore = lambda d,names: [ '.svn','.git' ]
            shutil.copytree(src, out, ignore=ignore)

            fp = os.path.join(out,'.project')
            print('ed:', fp, '<name>', prj.name)
            etree_replace_text(fp, './name', prj.name)

            if self.VARIANT == 'release':
                sf, df = os.path.join(src,self._Crypto0), os.path.join(out,self._Crypto1)
                print('copyfile:', sf, df)
                shutil.copyfile(sf, df)
                log_h = os.path.join(out,self._Log_h)
                print('ed:', log_h, 'BUILD_RELEASE')
                ed('^[^\s]+#\s*define\s+BUILD_RELEASE', '#define BUILD_RELEASE', log_h, count=1)

            if prj.name == 'Game16':
                fp = os.path.join(out, self._AndroidManifest)
                print('ed:', fp, 'android.uid.system')
                ed('\sandroid:versionName="(\d+\.\d+)"\s'
                        , ' android:versionName="\\1" android:sharedUserId="android.uid.system" '
                        , fp, count=1)
                #subprocess.check_call(command('cd mt6580 && git pull'), shell=True, executable='/bin/bash')

            if prj.name == 'Game14':
                for fp in glob.glob(os.path.join(out, 'doc/*.xls')):
                    print('copy:', fp)
                    shutil.copy(fp, '/samba/release1/doc/')
                with open('svn.log.{}.{}'.format(prj.name,prj.ver()), 'w') as f:
                    subprocess.check_call(command('svn log -r{}:HEAD {}', prj.svnrev(old=1), src)
                            , stdout=f, shell=True, executable='/bin/bash')
                    print(prj.fullver()
                            , '%s => %s' % (prj.svnrev(old=1),prj.svnrev())
                            , '%s => %s' % (prj.ver(old=1),prj.ver()), file=f)
                    print('$ vim', '~/release/Game14/doc/版本发布记录.txt', f.name)

            self.version_set(os.path.join(out,prj._AppConfig), prj.ver(), prj.svnrev())

        for prj in self.projects:
            impl(self, prj)
        print()
        self.show_info(*args, **kvargs)

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
                    return re.sub('=\s*"v[^"]+"', '= "v{}"'.format(ver), r.group(0))
                return re.sub('=\s*"v[^"]+"', '= "v{}-{}"'.format(ver, svnrev), r.group(0))
            for line in lines:
                lin = re.sub(re_ver, repf, line)
                if lin is line:
                    lin = re.sub(re_svnrev, repf, line)
                if not (lin is line):
                    print('ed:', appconfig, re.search('(\w+)\s*=\s*"([^"]+)', lin).groups())
                outf.write(lin)

    def build(self, *args, RARPWD=None, **kvargs):
        if self.VARIANT != 'release':
            return

        prj2 = None
        for prj in self.projects:
            for plt in prj.plats:
                print('>>>', plt, prj)
                if plt == 'g500':
                    subprocess.check_call(command('cd mt6580 && git pull'), shell=True, executable='/bin/bash')
                    sd, td = os.path.join(self.BUILD_DIR,prj.name), os.path.join(self.BUILD_DIR,'mt6580/alps/packages/apps/Game/')
                    for fp in 'libs/armeabi-v7a/libBarcode.so', 'libmtkhw.so':
                        fp = os.path.join(sd,fp)
                        print('copy:', fp)
                        shutil.copy(fp, td)
                    fp = os.path.join(self.BUILD_DIR,'release/{}.apk'.format(prj.fullver()))
                    print('copy:', fp)
                    shutil.copyfile(fp, os.path.join(td,'Game.apk'))
                    cmd = command('cd {0}/mt6580/alps'
                            ' && source build/envsetup.sh && lunch full_ckt6580_we_l-user && make -j8'
                            ' && mt6580-copyout.sh /samba/release1/{1}'
                            , self.BUILD_DIR, prj.platver(plt))
                    subprocess.check_call(cmd, shell=True, executable='/bin/bash')
                    if RARPWD:
                        self._make_archive(plt, prj, RARPWD)
                else:
                    prj2 = prj
        if prj2 and prj2.plats:
            cmd = command('cd {} && make -f Game14.mk PLATS={} release'
                    , self.BUILD_DIR, ','.join(prj2.plats))
            subprocess.check_call(cmd, shell=True, executable='/bin/bash')
            if RARPWD:
                for plt in prj2.plats:
                    self._make_archive(plt, prj, RARPWD)

    def _make_archive(self, plt, prj, rarpwd):
        cmd = command('cd /samba/release1'
                ' && ( rm -f {0}.rar ; rar a -hp{1} {0}.rar /samba/release1/{0} )'
                , prj.platver(plt), rarpwd)
        subprocess.check_call(cmd, shell=True, executable='/bin/bash')

    def rar(self, *args, RARPWD=None, **kvargs):
        assert RARPWD
        for prj in self.projects:
            for plt in prj.plats:
                self._make_archive(plt, prj, RARPWD)

    def sync_down(self, *args, **kvargs):
        assert RARPWD
        for plt in self.PLATS:
            for prj in self.projects:
                if plt in prj.plats:
                    rel = '{}/{}-{}-{}'.format(self.VARIANT, plt, prj.ver(), prj.datestr)
                    plt,prj
        #rel="$plat-$NewVer-`datestr`"
        #rsync -vrL $rhost:$variant/$rel $outdir/ || die "$rhost $rel"

    def version_commit(self, *args, **kvargs):
        #@contextlib.contextmanager
        #def commit(self, **kvargs):
        #    yield
        #    for prj in self.projects:
        #        message="Version{ExtraVersionInfo}({OldVer}=>{NewVer}, {OldSVNRev}=>{NewSVNRev}) updated".format(**kvargs)
        #        subprocess.check_call(command('cd {} && svn commit -m"{}"', src, message)
        #                , shell=True, executable='/bin/bash')
        prjs = []
        def revert(self):
            for prj,msg in prjs:
                subprocess.check_call(command('cd {} && svn revert {}', prj.repo, prj._AppConfig)
                        , shell=True, executable='/bin/bash')
        try:
            kvargs.setdefault('ExtraVersionInfo','')
            for prj in self.projects:
                d = prj.vars(**kvargs)
                msg = "Version{ExtraVersionInfo}({OldVer}=>{NewVer}, {OldSVNRev}=>{NewSVNRev}) updated".format(**d)
                print(msg)
                yN = input('commit %s? (y/N): ' % prj.repo)
                if yN.lower() not in ('y','yes'):
                    raise EOFError
                prjs.append( (prj,msg) )
            for prj,msg in prjs:
                self.version_set(os.path.join(prj.repo,prj._AppConfig), prj.ver(), prj.svnrev())
                subprocess.check_call(command('cd {} && svn commit -m"{}"', prj.repo, msg)
                        , shell=True, executable='/bin/bash')
        except (KeyboardInterrupt,EOFError):
            revert(self)
        except Exception:
            revert(self)
            raise

    def help(self, *args, **kvargs):
        print('''\
Usages:
    {0} prepare PLATS={sPLATS} VARIANT=[release|test]
    {0} build   PLATS={sPLATS} VARIANT=[release|test] RARPWD=XXX
    {0} version_commit
'''.format(sys.argv[0], sPLATS=','.join(Main.PLATS), **vars(self)))
        self.show_info(*args, **kvargs)

    def show_info(self, *args, **kvargs):
        pprint(vars(self))
        if args: pprint(args)
        if kvargs: pprint(kvargs)
        print()
        for prj in self.projects:
            print(prj.svnrev(old=1), '=>', prj.svnrev()
                    , prj.fullver(), '{}\\{}.apk'.format(self.VARIANT, prj.fullver()))
            #pprint(vars(self)) #pprint(globals())
        grep('Key.*word', 'howto.txt')

def command(s, *args, **kv):
    cmd = s.format(*args, **kv)
    print(cmd)
    return cmd

def etree_replace_text(filename, path, text):
    from xml.etree.ElementTree import ElementTree
    tree = ElementTree(file=filename)
    node = tree.find(path)
    if node != None:
        node.text = text
        tree.write(filename, encoding='UTF-8', xml_declaration=True) #(, short_empty_elements=False)

def ed(expr, ss, *files, count=-1):
    for textf in files:
        lines = []
        with open(textf) as sf:
            lines = sf.readlines()
        with open(textf, 'w') as outf:
            for line in lines:
                if count > 0:
                    l, line = line, re.sub(expr, ss, line)
                    if not (l is line):
                        count -= 1
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

def die(*args):
    #print(*args, file=sys.stderr)
    raise SystemExit(*args)
    sys.exit(127)

def _main_():
    import signal
    def _sig_handler(signal, frame):
        global _STOP
        _STOP = 1
    signal.signal(signal.SIGINT, _sig_handler)

    def _fn_lis_dic(args):
        fn, lis, dic = '', [], {} # defaultdict(list)
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
                if not fn:
                    fn = a
                else:
                    lis.append(a)
        return fn, lis, dic
    def _fn(fn):
        mod = sys.modules[__name__]
        f = getattr(mod, fn, None)
        if not f:
            cls = getattr(mod, 'Main', lambda *x,**y: None)
            f = getattr(cls(*args, **kvargs), fn, None)
        if not f:
            f = getattr(mod, 'help', None)
        if not f:
            raise RuntimeError(fn, 'not found')
        return f

    t0 = time.time()
    fn, args, kvargs = _fn_lis_dic(sys.argv[1:])
    global _FUNC
    _FUNC = fn
    _fn(fn)(*args, **kvargs)
    sys.stdout.flush()
    print('time({}): {}m{}s'.format(fn, *map(int, divmod(time.time() - t0, 60))), file=sys.stderr)

if __name__ == '__main__':
    try:
        _main_()
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

