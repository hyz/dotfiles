
from http.server import HTTPServer,BaseHTTPRequestHandler
import shutil, struct

class MyHttpHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # for x in dir(self): print (x, getattr(self,x)) ; print (self.path)
        if self.path.startswith('/?'):
            import io, os
            rf = io.BytesIO(bytes('\n'.join(os.listdir(self.path[2:])),'GB2312'))
        else:
            rf = open('.'+self.path, 'rb')
        self.send_response(200)
        self.send_header("Content-type", "text/plain; charset=gb2312")
        self.send_header("Content-Length", rf.seek(0,2))
        self.end_headers()
        rf.seek(0)
        shutil.copyfileobj(rf, self.wfile)
        rf.close();
        self.finish()

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
    httpd=HTTPServer(('', 8000),MyHttpHandler)
    print("Server started on 0.0.0.0:8000 ...")
    httpd.serve_forever()

