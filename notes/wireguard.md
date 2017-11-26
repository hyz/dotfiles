
### https://wiki.archlinux.org/index.php/WireGuard

    wg genkey | tee wg.genkey | wg pubkey > wg.pubkey

### https://github.com/alokhan/wireguard/blob/master/command

Server setup:

    ip link add dev wg0 type wireguard
    ip address add dev wg0 192.168.2.1 peer 192.168.2.2
    sudo wg set wg0 listen-port 1122 peer publickeypeer allowed-ips 192.168.2.0/2
    ip link set up dev wg0

    sudo iptables -A FORWARD -i eth0 -o wg0 -m state --state ESTABLISHED,RELATED -j ACCEPT
    sudo iptables -A FORWARD -s 192.168.2.0/24 -o eth0 -j ACCEPT
    sudo iptables -t nat -A POSTROUTING -s 192.168.2.0/24 -o eth0 -j MASQUERADE


Client Setup:

    ip link add dev wg0 type wireguard
    ip address add dev wg0 192.168.2.2 peer 192.168.2.1

    # allowed ips 0.0.0.0/0 is to enable forwarding all traffic throw the link
    sudo wg set wg0 listen-port 1122 private-key privatekey peer publickeypeer allowed-ips 0.0.0.0/0  endpoint serverpublicip:1122

    ip link set up dev wg0

    Forward traffic throw wg0:

    sudo ip route add publicipserver via 192.168.1.1 dev enp3s0 proto static
    sudo ip route change default via 192.168.2.2 dev wg0 proto static

