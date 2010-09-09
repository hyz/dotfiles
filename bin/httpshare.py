#!/usr/bin/python3

from http.server import HTTPServer,BaseHTTPRequestHandler
import shutil, struct

class MyHttpHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        try:
            rf = open('html%s' % self.path, 'rb')
        except:
            rf = open('./%s' % self.path, 'rb')
        #with open('html%s' % self.path, 'rb') as rf:
        self.send_response(200)
        self.send_header("Content-type", "text/plain; charset=gb2312")
        self.send_header("Content-Length", rf.seek(0,2))
        self.end_headers()
        rf.seek(0)
        shutil.copyfileobj(rf, self.wfile)
        rf.close();

    def do_POST(self):
        cmd = 0
        try:
            clen = int(self.headers['Content-Length'])
            print('Content-Length:', clen)
            buf = self.rfile.read(clen)
            cmd = struct.unpack('!h', buf[:2])
            print('cmd=%d' % cmd)
            with open('html/post/%d.req' % cmd, 'wb') as wf:
                wf.write(buf)
            with open('html/%d.resp' % cmd, 'rb') as rf:
                self.send_response(200)
                self.send_header("Content-type", "application/octet-stream")
                self.send_header("Content-Length", rf.seek(0,2))
                self.end_headers()
                rf.seek(0)
                shutil.copyfileobj(rf, self.wfile)
        except:
            self.send_error(404, 'cmd=%d' % cmd)

if __name__ == '__main__':
    httpd=HTTPServer(('', 80),MyHttpHandler)
    print("Server started on 127.0.0.1,port 80.....")
    httpd.serve_forever()

