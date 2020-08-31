
https://wiki.archlinux.org/index.php/Bluetooth_headset
https://wiki.archlinux.org/index.php/Bluetooth#Audio
https://wiki.archlinux.org/index.php/PulseAudio


    lsmod |grep blue
    journalctl -b | grep Bluetooth
    lspci -k -s 02:00.0


	systemctl restart bluetooth.service
    systemctl status  bluetooth.service
    rfkill
    rfkill unblock bluetooth

    bluetoothctl list
    bluetoothctl power on
    bluetoothctl default-agent

    bluetoothctl devices
    bluetoothctl paired-devices

    bluetoothctl info XX:YY:ZZ:...
    bluetoothctl trust XX:YY:ZZ:..
    bluetoothctl connect XX:YY:ZZ:...

    bluetoothctl show

    bluetoothctl disconnect


