
### https://wiki.archlinux.org/index.php/Internet_sharing#Enable_packet_forwarding

    sysctl -a | grep forward
    sudo sysctl net.ipv4.ip_forward=1

### vim /etc/sysctl.d/30-ipforward.conf

    net.ipv4.ip_forward=1
    net.ipv6.conf.default.forwarding=1
    net.ipv6.conf.all.forwarding=1

### ip

    ip l                                                                                                                                                      ~ 6:30

1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: enp1s0f0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 00:1b:21:d6:e1:06 brd ff:ff:ff:ff:ff:ff
3: enp3s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP mode DEFAULT group default qlen 1000
    link/ether d4:3d:7e:00:e9:18 brd ff:ff:ff:ff:ff:ff
4: enp1s0f1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP mode DEFAULT group default qlen 1000
    link/ether 00:1b:21:d6:e1:07 brd ff:ff:ff:ff:ff:ff

    ip r                                                                                                                                                      ~ 6:30

default via 192.168.1.1 dev enp3s0
172.16.1.0/24 dev enp1s0f1 proto kernel scope link src 172.16.1.23
192.168.1.0/24 dev enp3s0 proto kernel scope link src 192.168.1.233
192.168.1.0/24 dev enp1s0f0 proto kernel scope link src 192.168.1.234

    iptables -A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT                                                                             ~ 6:31
    iptables -A FORWARD -i enp1s0f1 -o enp3s0 -j ACCEPT

    iptables -A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT                                                                             ~ 6:31
    iptables -A FORWARD -i enp1s0f1 -o enp3s0 -j ACCEPT


### bridge

    cp /etc/netctl/examples/bridge /etc/netctl/

