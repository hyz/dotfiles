#!/usr/bin/env python

import sys

paste_url = 'http://paste.ubuntu.org.cn'

boundary='----tHe[BoUnDaRy]_$'

def new_content(mimetype, cont, name='', filename=''):
    print mimetype, cont, name, filename
    cont = []
    if filename:
        if mimetype.startswith('text/'):
            cont = new_content('text/plain', 'bash', name='class') # 
            name = 'code2'
        elif mimetype.startswith('image/'):
            name = 'screenshot'
        if filename == '-':
            filename = ''
        else:
            filename = '; filename=%s' % filename
    if name:
        cont.append('--' + boundary)
        cont.append('Content-Disposition: form-data; name="%s"' % name + filename)
        cont.append('Content-Type: %s' % mimetype)
        cont.append('')
        cont.append(cont)
    return cont

def post(l, **d):
    import magic
    mcont = []
    for k, v in d.items():
        mcont += new_content('text/plain', v, name=k)
    for k, v in l:
        mcont += new_content(magic.from_buffer(v, mime=True) or 'application/octet-stream', v, filename=k)
    if mcont:
        import httplib
        hdrs = {'Content-Type':"multipart/form-data; boundary=%s" % boundary}
        cont = "\r\n".join(mcont) + '\r\n--'+boundary+'--\r\n'
        print cont
        return ''
        http = httplib.HTTPConnection(paste_url)
        http.request("POST", "/", cont, hdrs)
        resp = http.getresponse()
        if resp.status == 302:
            return paste_url + resp.getheader('Location')
        else:
            return 'Error Response: ', resp.status

def main():
    import optparse
    parser = optparse.OptionParser()
    parser.add_option("-u", "--poster", dest="poster", default="")
    parser.add_option("-p", "--paste", dest="paste", default="Send")
    opts, args = parser.parse_args()
    l = []
    for x in args:
        if x == '-':
            l.append((x, sys.stdin.read()))
        else:
            l.append((x, file(x, 'rb').read()))
    print post(l, poster=opts.poster, paste=opts.paste)

if __name__ == "__main__":
    main()

