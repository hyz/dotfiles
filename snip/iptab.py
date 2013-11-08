#!/usr/bin/env python

# 
# a.out A 253 192.168.3.2

import sys
import socket, struct

opc = len(sys.argv) > 1 and sys.argv[1] or 'A'

count = len(sys.argv) > 2 and int(sys.argv[2]) or 1
a_ip = len(sys.argv) > 3 and sys.argv[3] or "192.168.3.1"
a_mac = len(sys.argv) > 4 and sys.argv[4] or "00:E0:6F:10:C0:A8"
# 6C:F0:49:6A:52:68

fmt1 = "iptables -t mangle -%s WiFiDog_br-lan_Outgoing -s %s -m mac --mac-source %s -j MARK --or-mark 0x0200"
fmt2 = "iptables -t mangle -%s WiFiDog_br-lan_Incoming -d %s -j ACCEPT"

aint = struct.unpack('>L', socket.inet_aton(a_ip))[0]
mac = a_mac.split(':')

while count > 0:
    if aint & 0xff == 0:
        aint |= 1
        if aint & 0xff00 == 0:
            aint |= 0x100
            if aint & 0xff0000 == 0:
                aint |= 0x10000

    n4 = list(struct.pack('>L', aint))
    s4 = [ str(x) for x in n4 ]
    h3 = [ '%02x' % x for x in n4[1:] ]
    # print(n4,s4,h3)

    # print(fmt1 % (opc, '.'.join(s4), ':'.join(mac[:-3] + h3) ))
    print(fmt1 % (opc, '.'.join(s4), a_mac))
    print(fmt2 % (opc, '.'.join(s4)))

    aint += 1
    count -= 1

# iptables -t mangle -D WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source EC:55:F9:A5:B8:DF -j MARK --or-mark 0x0200
# iptables -t mangle -A WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source 00:E0:6F:10:C0:A8 -j MARK --or-mark 0x0200
# iptables -t mangle -D WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT 
# iptables -t mangle -A WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT
# iptables -t mangle -A WiFiDog_br-lan_Outgoing -s 192.168.3.234 -m mac --mac-source 00:E0:6F:10:C0:A8 -j MARK --or-mark 0x0200
# iptables -t mangle -A WiFiDog_br-lan_Incoming -d 192.168.3.234 -j ACCEPT

#hexs = "0123456789ABCDEF"
#hexbytes = [ x+y for x in hexs for y in hexs][1:-1]
#
## print(opc,a_ip,a_mac, hexbytes)
#
#def _ylis(lis, n, l):
#    if n == 1:
#        for y in hexbytes:
#            lis.append(l + [y])
#    elif n > 1:
#        for x in hexbytes:
#            _ylis(lis, n-1, l + [x])
#
#def ylis(n):
#    lis = []
#    _ylis(lis, n, [])
#    return lis
#
# for l in ylis(npart):
#     ip = '.'.join( a_ip.split('.')[:-len(l)] + [ str(int(x,16)) for x in l ] )
#     mac = ':'.join( a_mac.split(':')[:-len(l)] + l )
# 
#     print(fmt1 % (opc, ip, mac))
#     print(fmt2 % (opc, ip))

