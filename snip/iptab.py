#!/usr/bin/env python

import sys

count = 1

opc = len(sys.argv) > 1 and sys.argv[1] or 'A'
a_ip = len(sys.argv) > 2 and sys.argv[2] or "192.168.3.1"
a_mac = len(sys.argv) > 3 and sys.argv[3] or "00:E0:6F:10:C0:A8"

# iptables -t mangle -D WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source EC:55:F9:A5:B8:DF -j MARK --or-mark 0x0200
# iptables -t mangle -A WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source 00:E0:6F:10:C0:A8 -j MARK --or-mark 0x0200
# iptables -t mangle -D WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT 
# iptables -t mangle -A WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT
fmt1 = "iptables -t mangle -%s WiFiDog_br-lan_Outgoing -s %s -m mac --mac-source %s -j MARK --or-mark 0x0200"
fmt2 = "iptables -t mangle -%s WiFiDog_br-lan_Incoming -d %s -j ACCEPT"

hexs = "0123456789ABCDEF"
hexbytes = [ x+y for x in hexs for y in hexs][1:-1]

# print(opc,a_ip,a_mac, hexbytes)

def ylis(lis, n, l=[]):
    if n == 1:
        for y in hexbytes:
            lis.append(l + [y])
    elif n > 1:
        for x in hexbytes:
            ylis(lis, n-1, l + [x])

lis = []
ylis(lis, count)
for l in lis:
    ip = '.'.join( a_ip.split('.')[:-len(l)] + [ str(int(x,16)) for x in l ] )
    mac = ':'.join( a_mac.split(':')[:-len(l)] + l )

    print(fmt1 % (opc, ip, mac))
    print(fmt2 % (opc, ip))

# iptables -t mangle -A WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source 00:E0:6F:10:C0:A8 -j MARK --or-mark 0x0200
# iptables -t mangle -A WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT

