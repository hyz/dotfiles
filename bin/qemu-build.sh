#!/bin/bash

#[ "`id -ru`" = "`id -ru build`" ]
[[ "`id -ru`" = 0 ]] || exit

tapX=tap0

exec 1>/dev/null
exec 2>/dev/null

if /sbin/ip link show dev $tapX ; then
     hda=/dev/sda5
     hdb=/dev/sda6
     Mac=52:54:00:12:32:64
     [ "`stat -c '%G' $hda`" = "kvm" ] || chgrp kvm $hda
     [ "`stat -c '%G' $hdb`" = "kvm" ] || chgrp kvm $hdb
     if pidof qemu-system-x86_64 ; then
       true
     else
       usermod -G kvm -a build
       sudo -iu build -- /usr/bin/qemu-system-x86_64 -name A1 -enable-kvm -machine type=pc,accel=kvm \
         -cpu host -smp cores=$(nproc) -m 4G \
         -drive file=$hda,index=0,media=disk,format=raw,if=virtio \
         -drive file=$hdb,index=1,media=disk,format=raw,if=virtio \
         -device virtio-net-pci,netdev=nwk1,mac=$Mac -netdev tap,id=nwk1,ifname=$tapX,script=no,downscript=no \
         -nographic &
     fi
else
     eth0=eth0
     br0=br0
     ip link set $eth0 up promisc on
     modprobe tun
     ip tuntap add dev $tapX mode tap user build
     ip link set $tapX master $br0 # brctl addif $br0 $tapX
     ip link set $tapX up # promisc on
fi

exit

