#!/usr/bin/env python

### https://stackoverflow.com/questions/6270782/how-to-send-an-email-with-python
### https://stackoverflow.com/questions/3362600/how-to-send-email-attachments-with-python
### http://www.pythonforbeginners.com/code-snippets-source-code/using-python-to-send-email
### https://stackoverflow.com/questions/24077314/how-to-send-an-email-with-style-in-python3
import sys, os, time
import argparse, getpass
import smtplib, email.utils
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

def main():
    def options():
        argp = argparse.ArgumentParser('Send email')
        argp.add_argument('-p', '--password', default=None, help='password') #(, nargs=2)
        argp.add_argument('-s', '--subject', default=time.strftime('%F') + ' <时间简史>')
        argp.add_argument('-f', '--from', dest='From', help='From: zero@qq.com')
        argp.add_argument('-a', '--attach', default=None)
        argp.add_argument('Tos', nargs='+', help='To: one@qq.com two@163.com')
        return argp.parse_args()

    opt = options()
    if not opt.password:
        opt.password = getpass.getpass('password:')

    multipart = MIMEMultipart()
    multipart['Subject'] = opt.subject
    multipart['From'] = opt.From
    multipart['To'] = email.utils.COMMASPACE.join(opt.Tos)

    body = sys.stdin.read()
    multipart.attach( MIMEText(body, 'html') )

    if opt.attach:
        for fn in opt.attach.split(','):
            if not os.path.exists(fn):
                print('file not exist:', fn, file=sys.stderr)
                continue
            basename = os.path.basename(fn)
            body = open(fn)
            part = MIMEText(body.read(), 'plain')
            body.close()
            part['Content-Disposition'] = f'attachment; filename="{basename}"'
            multipart.attach( part )

    #multipart.preamble = 'One small step for man, one giant stumble for mankind.'
    #for file in pngfiles:
    #    with open(file, 'rb') as fp:
    #        img = MIMEImage(fp.read())
    #    multipart.attach(img)

    smtp = smtplib.SMTP_SSL('smtp.ym.163.com',465)
    smtp.login(opt.From, opt.password)
    smtp.sendmail(opt.From, opt.Tos, multipart.as_string())
    smtp.quit()
main()

### MIMEMultipart
#
# Received: from mr213139.mail.yeah.net (unknown [223.252.213.139])
# 	by newmx100.qq.com (NewMx) with SMTP id 
# 	for <jywww@qq.com>; Tue, 20 Jun 2017 14:31:19 +0800
# X-QQ-FEAT: IoaPf3hsqvgNTdNBwlvEH8BCpTue/ekC6ufoDg8yuniDziQnNpd6eok2zLzzM
# 	qJM5MRfoHVzAgU327+u0uTpOLuYCf/eZQtMqr6apR2mDOU5dibTkIgqeVDkuuFTb5UUqaWI
# 	0mQY9CKTJlmLA9p/D7HbKEkx6vqd4oJYeJgIexS7S3tYgy5g9zfrfxz1sOKB7q7wlczuwUH
# 	zFGkLpZSBodktTNragMQyIVQHqSV838M=
# X-QQ-MAILINFO: MjJD59SVx+LnlHwzH/gViZOHg6CqfyClp7MtHLBFnXR0yy46d2PacQGen
# 	roVpM26xa+KkczxZGDpUY9/VUFQl+AI1qLL2KqcrZIY/10qNAvUl4ngfHeuEa83YfQIHM3W
# 	A7jer8PB3fhPSSB55LsipyAm91TBWvK+fnb1IICJSTrNGUlaG3ktm1M=
# X-QQ-mid: mx100t1497940279t1dnw1hum
# X-QQ-ORGSender: wujiyong@huaguanjiye.com
# Received: from localhost.localdomain (unknown [113.91.141.200])
# 	by mr213139.mail.yeah.net (HMail) with ESMTPSA id A2BEC1C1657
# 	for <jywww@qq.com>; Tue, 20 Jun 2017 14:31:18 +0800 (CST)
# Content-Type: multipart/mixed; boundary="===============4522722513477787035=="
# MIME-Version: 1.0
# Subject: =?utf-8?b?5pe26Ze0566A5Y+y?=
# From: wujiyong@huaguanjiye.com
# To: jywww@qq.com
# X-HM-Spam-Status: e1ktWUFJV1koWUFPN1dZCBgUCR5ZQUtVS1dZCQ4XHghZQVkyNS06NzI*QU
# 	tVS1kG
# Message-Id: <20170620143118.A2BEC1C1657@mr213139.mail.yeah.net>
# Date: Tue, 20 Jun 2017 14:31:18 +0800 (GMT+08:00)
# X-HM-Sender-Digest: e1kSHx4VD1lBWUc6MBQ6Shw4LToiPTAISwM3HwI5KjMwCx1VSlVKT0JM
# 	Qk9LSUxDTE5LVTMWGhIXVQwOERICFBUcOxMOGhwOGhUREgIeVRgUFkVZV1kMHhlZQR0aFwgeV1kI
# 	AVlBT0hKN1dZEgtZQVlKSkhVQkpVSk9KVUlLS1kG
# X-HM-Tid: 0a5cc432bda77d8bkuuka2bec1c1657
# 
# One small step for man, one giant stumble for mankind.
# --===============4522722513477787035==
# 
# --===============4522722513477787035==--

### MIMEText
# Received: from m199-177.yeah.net (unknown [123.58.177.199])
# 	by newmx110.qq.com (NewMx) with SMTP id 
# 	for <jywww@qq.com>; Tue, 20 Jun 2017 14:36:15 +0800
# X-QQ-FEAT: r4zXc+C+CK4jSJAsA7HCiCduB1uedaQWIa3/X+I1Mixrn0VuURcu2IZoHIbWr
# 	X8KS6tSKpgTVRleuAWT6+rg1h0utB1r/VgoC5wwcky5qprIaawF/rAQRClcm1u50WZY2Sa2
# 	CplF3/yYdR44BohXQOVqPumvLLTvFUjd1VaKUHMOEJZworRQjMKXYHLq8PEpd3hWGEUAyI+
# 	ca4+RniIO/tdUr0EUHk3zwyTj3nlx8SU9B0xYqYJSpRhSXL3CW7LJQSqW3quKDbw=
# X-QQ-MAILINFO: ODvMQTwIxEi37p/HMsUolFdaca6fcm5/AV0NqtHlhq4TfcMfZ5kRnT+o2
# 	rJ2NwMiN0WHpXtLDnaTHALnIyR1dMPtXv540xwLHirDLGmQz7t19jfp7tuC6nJ+7QVxz31B
# 	p/X1S/vO3l57XhD00Q6qpIA3kEk0vnpJsbHm4ReGrrRou/b1nmCdJNa5Nq2vnArAoA==
# X-QQ-mid: mx110t1497940576t7jaxdx8o
# X-QQ-ORGSender: wujiyong@huaguanjiye.com
# Received: from localhost.localdomain (unknown [113.91.141.200])
# 	by m199-177.yeah.net (HMail) with ESMTPSA id 88E121104078
# 	for <jywww@qq.com>; Tue, 20 Jun 2017 14:36:15 +0800 (CST)
# Content-Type: text/plain; charset="us-ascii"
# MIME-Version: 1.0
# Content-Transfer-Encoding: 7bit
# Subject: =?utf-8?b?5pe26Ze0566A5Y+y?=
# From: wujiyong@huaguanjiye.com
# To: jywww@qq.com
# X-HM-Spam-Status: e1ktWUFJV1koWUFPN1dZCBgUCR5ZQUtVS1dZCQ4XHghZQVkyNS06NzI*QU
# 	tVS1kG
# Message-Id: <20170620143615.88E121104078@m199-177.yeah.net>
# Date: Tue, 20 Jun 2017 14:36:15 +0800 (GMT+08:00)
# X-HM-Sender-Digest: e1kSHx4VD1lBWUc6NCI6SAw*KToaAzAUS0oCPjdJHjUKCTJVSlVKT0JM
# 	Qk9LTkxOTU5NVTMWGhIXVQwOERICFBUcOxMOGhwOGhUREgIeVRgUFkVZV1kMHhlZQR0aFwgeV1kI
# 	AVlBSEhPN1dZEgtZQVlKSkhVQkpVSk9KVUlLS1kG
# X-HM-Tid: 0a5cc43745666427kurs88e121104078
# 
# Rainy days and Mondays always get me down.

