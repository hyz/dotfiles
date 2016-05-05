#!/usr/bin/env python3
 
'''Simple HTTP Server With Upload.

This module builds on BaseHTTPServer by implementing the standard GET
and HEAD requests in a fairly straightforward manner.

see: https://gist.github.com/UniIsland/3346170
'''
 
 
__version__ = "0.1"
__all__ = ["simple-py3httpd"]
__author__ = "woody"
__home_page__ = "https://github.com/hyz/"
 
import os, sys, io
import re
#import posixpath
import http.server
import urllib.request, urllib.parse, urllib.error
import cgi
import shutil
import mimetypes
import html
import subprocess
 
Null = open(os.devnull) #_Empty()
_xls_txt = 'sheet.txt'
_xlsprint = '../bin/xlsprint'

class Part(object):
    #'Content-Disposition' #'application/octet-stream'

    def __init__(self): #(self, name=None, filename=None, body=None, headers={}):
        #self.headers = None
        self.name = None
        self.filename = None
        self.body = None # io.BytesIO()
        self._last = None

    def parse(self, line):
        if self.body is None:
            if line == b'\r\n':
                self.body = [] #io.BytesIO()
            else:
                self._parse_header(line)
        else:
            if self._last:
                self.body.append(self._last)
            self._last = line

    def end(self):
        if self._last:
            assert self._last.endswith(b'\r\n')
            la = self._last[:-2]
            if la:
                self.body.append(la)
            del self._last
        return self

    def __str__(self):
        return str(self.__dict__)

    def _parse_header(self, line):
        m = re.match(b'^Content-Disposition.*name="(.*)"; filename="(.*)"\r\n$', line)
        if m:
            self.name     = html.unescape(m.group(1).decode())
            self.filename = html.unescape(m.group(2).decode())
        #if self._filename == None:
        #    self._headers[Part.CONTENT_DISPOSITION] = ('form-data; name="%s"' % self._name)
        #else:
        #    self._headers[Part.CONTENT_DISPOSITION] = ('form-data; name="%s"; filename="%s"' % (self._name, self._filename))

def multipart(rfile, content_type):
    boundary = rfile.readline().strip()

    m = re.match(r'^multipart/form-data;\s*boundary=(.*)$', content_type)
    if not m:
        raise ValueError('Content_Type invalid') 
    if not re.match(b'^-*' + m.group(1).encode() + b'-*$', boundary):
        raise ValueError('multipart/form-data boundary mismatch') 

    part = Part()
    for line in rfile:
        if line.startswith(boundary):
            part.end()
            yield part
            part = Part()
        part.parse(line)

class SimpleHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    server_version = "SimpleHTTPWithUpload/" + __version__

    #parameter_list ::=
    #    (defparameter ",")*
    #    ( "*" [parameter] ("," defparameter)* [, "**" parameter]
    #       | "**" parameter
    #       | defparameter [","] )

    def print(self, *args, **kwargs):
        sep = kwargs.pop('sep', '')
        file = kwargs.pop('file', self.wfile)
        __builtins__.print(*args, sep=sep, file=file, **kwargs)

    def format_html(self, word, th, rows):
        def print_row(row, file=self.wfile):
            print('<tr>', file=file)
            for i,col in enumerate(row):
                print(('<td>','<td align="right">')[i==2], file=file)
                idx = col.rfind(word)
                if idx >= 0:
                    print(col[0:idx], '<font color="#990012"><u>', word, '</u></font>', col[idx+len(word):], file=file, sep='')
                    #print(col[0:idx], '<u>', word, '</u>', col[idx+len(word):], file=file, sep='')
                else:
                    print(col, file=file)
                print('</td>', file=file)
            print('</tr>', file=file)
            #print('<li><a href="%s">%s</a>' % (urllib.parse.quote(linkname), cgi.escape(displayname)), file=f)
        with io.StringIO() as f:
            print('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">', file=f)
            print('<HTML>', '<title>Search %s</title>' % word, '<BODY>', file=f)
            #print('<h2></h2>' % displaypath, '<hr/>', file=f)

            print('<FORM method="GET" action="/?">', file=f)
            print('<INPUT type="text" name="k" value="%s"/>' % word, file=f)
            print('<INPUT type="submit" value="search"/>', file=f)
            print('</FORM>', file=f)

            print('<HR>', '<TABLE border="1" cellspacing="0">', file=f)
            print_row(th, file=f)
            for row in rows:
                print_row(row, file=f)
            print('</TABLE>', '<HR>', file=f)

            print('<FORM ENCTYPE="multipart/form-data" method="POST">', file=f)
            print('<INPUT name="file" type="file"/>', file=f)
            print('<INPUT type="submit" value="upload"/>', file=f)
            print('</FORM>', file=f)

            print("</BODY>", "</HTML>", file=f)

            utf8 = f.getvalue().encode()
        return utf8

    def grep(self, word):
        #out, _ = subprocess.Popen(['grep', word, _xls_txt], stdout=subprocess.PIPE).communicate()
        with open(_xls_txt) as f:
            th = f.readline().split('\t')
            assert len(th) > 2
            rows, tmps, pos = [], [], 0
            for i,line in enumerate(f,1): #out.decode().split('\n'): #str(out, 'UTF-8')
                r = line.split('\t') 
                if len(r) != len(th):
                    continue
                if r[2].endswith(word):
                    # kpos2 = len(r[2]) - r[2].rfind(word)
                    # if kpos2 < len(r[2]): # or len(r[6]) - r[6].rfind(word) <= len(r[6]):
                    # rows.append(r)
                    rows += tmps
                    tmps = []
                    pos = i+4
                    rows.append(r)
                elif i <= pos:
                    rows.append(r)
                else:
                    if len(tmps) >= 3:
                        tmps.pop(0)
                    tmps.append(r)
            # rows.sort(key=lambda r: len(r[2])-r[2].rfind(word))#(, reverse=True)
            return (th, rows)
 
    def do_GET(self):
        '''Serve a GET request.'''
        path = self.real_path()

        if self.path.startswith('/?') and self.querys:
            k = self.querys.get('k').strip()
            if k:
                utf8 = self.format_html(k, *self.grep(k))
                self.send_response(200)
                self.send_header("Content-type", "text/html; charset=UTF-8")
                self.send_header("Content-Length", str(len(utf8)))
                #self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
                self.end_headers()
                self.wfile.write(utf8)
                return

        self._get_headers(path)
 
    def do_HEAD(self):
        '''Serve a HEAD request.'''
        self._get_headers(self.real_path())
 
    def do_POST(self):
        with io.StringIO() as f: # io.BytesIO()
            suc, info = self._post_file()
            suc = ('Failed','Success')[int(suc)]

            print('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">', file=f)
            print('<HTML>', '<TITLE>Upload Result Page</TITLE>', '<BODY>', file=f)
            print('<h2>Upload Result Page</h2>', file=f)
            print('<HR/>', '<strong>%s</strong>' % suc, info, file=f)
            print('<BR/>', '<a href="%s">back</a>' % self.headers['referer'], file=f)
            print('<HR/>', '<small>Powerd By: <a href="https://github.com/hyz">woody</a>.</small>', file=f)
            print('</BODY>', '</HTML>', file=f)

            content = f.getvalue().encode('UTF-8')

            self.send_response(200)
            self.send_header("Content-type", "text/html; charset=UTF-8")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content) # self.copyfile(f, self.wfile)
        
    def _post_file(self):
        filename = None
        for part in multipart(self.rfile, self.headers['content-type']):
            part.filename = os.path.normpath(part.filename).strip('./\\')
            if part.filename:
                filename = part.filename
                fullp = self.real_path(filename)
                try:
                    with open(fullp, 'wb') as out:
                        for b in part.body:
                            out.write(b)
                    self.simplyfied_table(fullp)
                    return (True, 'Upload success: %s' % filename)
                except IOError:
                    return (False, "Upload fail: file=%s" % fullp)
        return (False, 'Upload fail: %s' % filename)
 
    def simplyfied_table(self, fullp):
        def _readlink(lnk):
            try:
                return os.readlink(lnk)
            except:
                return None
        def _unlink(pa):
            try:
                os.unlink(pa)
            except:
                pass
        tmpfn = '/tmp/taobao-helper.xls.txt'
        with open(tmpfn, 'w') as out:
            if 0 == subprocess.call([_xlsprint, fullp], stdout=out):
                lnk_xls = '.xls'
                prev_xls = _readlink(lnk_xls)
                if prev_xls:
                    if prev_xls != fullp:
                        _unlink(prev_xls)
                        _unlink(lnk_xls)
                        os.symlink(fullp, lnk_xls)
                else:
                    os.symlink(fullp, lnk_xls)
                _unlink(_xls_txt)
                os.rename(tmpfn, _xls_txt)

    def _get_headers(self, path):
        if os.path.isdir(path):
            #if not self.path.endswith('/'):
            #    # redirect browser - doing basically what apache does
            #    self.send_response(301)
            #    self.send_header("Location", self.path + "/")
            #    self.end_headers()
            #    return Null
            for index in "index.html", "index.htm":
                index = os.path.join(path, index)
                if os.path.exists(index):
                    path = index
                    break
            else:
                return self._do_list_directory(path)

        ctype = self.guess_type(path)
        try:
            f = open(path, 'rb')
            fs = os.fstat(f.fileno())
            self.send_response(200)
            self.send_header("Content-type", ctype)
            self.send_header("Content-Length", str(fs.st_size))
            self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
            self.end_headers()
            shutil.copyfileobj(f, self.wfile)

        except IOError:
            self.send_error(404, "File not found")
            return Null
 
    def _do_list_directory(self, path):
        try:
            for _, dirs, files in os.walk(path): break
            list = [ x for x in dirs + files if not x.startswith('.') ]
            # list = [ x for x in os.listdir(path) if not x.startswith('.') ]
        except os.error:
            self.send_error(404, "No permission to list directory")
            return Null
        list.sort(key=lambda a: a.lower())
        displaypath = cgi.escape(urllib.parse.unquote(self.path))
        with io.StringIO() as f:
            print('<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">', file=f)
            print('<HTML>', '<title>Directory listing for %s</title>' % displaypath, '<BODY>', file=f)
            print('<h2>Directory listing for %s</h2>' % displaypath, '<hr/>', file=f)
            print('<FORM ENCTYPE="multipart/form-data" method="POST">', file=f)
            print('<INPUT name="file" type="file"/>', file=f)
            print('<INPUT type="submit" value="upload"/>', file=f)
            print('</FORM>', '<hr><ul>', file=f)
            for name in list:
                fullname = os.path.join(path, name)
                displayname = linkname = name
                # Append / for directories or @ for symbolic links
                if os.path.isdir(fullname):
                    displayname = name + "/"
                    linkname = name + "/"
                if os.path.islink(fullname):
                    displayname = name + "@"
                    # Note: a link to a directory displays with @ and links with /
                print('<li><a href="%s">%s</a>' % (urllib.parse.quote(linkname), cgi.escape(displayname)), file=f)
            print('</ul><hr>', file=f)

            print('<FORM method="GET" action="/?">', file=f)
            print('<INPUT type="text" name="k"/>', file=f)
            print('<INPUT type="submit" value="search"/>', file=f)
            print('</FORM>', file=f)

            print('</BODY>', '</HTML>', file=f)

            content = f.getvalue().encode('UTF-8')
        #length = f.tell() f.seek(0)
        #f.close()

        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=UTF-8")
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content) # self.copyfile(f, self.wfile)

    def real_path(self, tail=None): # translated_path(self):
        #def make_querys(querys):
        #    m = {}
        #    for p in querys.split('&'):
        #        k,_,v = p.partition('=')
        #        k = urllib.parse.unquote(k).strip()
        #        v = urllib.parse.unquote(v).strip()
        #        if k and v:
        #            m[k] = v
        #    return m
        path, _, qsl = self.path.partition('?')
        if qsl:
            self.querys = dict( urllib.parse.parse_qsl(qsl) ) # self.querys = make_querys(self.querys)
        if path is self.path:
            path,_,_ = path.partition('#')
        path = os.path.normpath(urllib.parse.unquote(path)).strip('./\\') # posixpath
        if tail:
            path = os.path.join(path, tail)
        return path or '.'
        #return os.path.join(os.getcwd(), path.strip('/'))

        #words = path.split('/')
        #words = [_f for _f in words if _f]
        #path = os.getcwd()
        #for word in words:
        #    drive, word = os.path.splitdrive(word)
        #    head, word = os.path.split(word)
        #    if word in (os.curdir, os.pardir):
        #        continue
        #    path = os.path.join(path, word)
        #return path
 
    def copyfile(self, source, outputfile):
        shutil.copyfileobj(source, outputfile)
 
    def guess_type(self, path):
        base, ext = os.path.splitext(path) # posixpath
        if ext in self.extensions_map:
            return self.extensions_map[ext]
        ext = ext.lower()
        if ext in self.extensions_map:
            return self.extensions_map[ext]
        else:
            return self.extensions_map['']
 
    if not mimetypes.inited:
        mimetypes.init() # try to read system mime.types

    extensions_map = mimetypes.types_map.copy()
    extensions_map.update({
        '': 'application/octet-stream', # Default
        '.py': 'text/plain',
        '.c': 'text/plain',
        '.h': 'text/plain',
        })
 
def test(HandlerClass = SimpleHTTPRequestHandler, ServerClass = http.server.HTTPServer):
    http.server.test(HandlerClass, ServerClass)

def ch_cwd():
    wd = os.path.join( os.getenv('HOME'), 'www' )
    if not os.path.isdir(wd):
        os.makedirs(wd, exist_ok=True)
    os.chdir(wd)

def main():
    ch_cwd()
    print('cwd', os.getcwd())
    httpd = http.server.HTTPServer(('', 80), SimpleHTTPRequestHandler)
    if os.getuid() == 0:
        os.setegid(1000)
        os.seteuid(1000)
    httpd.serve_forever()
 
if __name__ == '__main__':
    # work_dir = ('.',sys.argv[1])[sys.argc>1 and os.path.isdir(sys.argv[1])]
    main()

