
### https://askubuntu.com/questions/372607/how-to-create-a-bootable-ubuntu-usb-flash-drive-from-terminal

    sudo dd bs=4M if=ubuntu-14.04.5-server-amd64.iso of=/dev/sdb conv=fdatasync

