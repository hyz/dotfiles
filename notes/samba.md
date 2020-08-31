
### http://unix.stackexchange.com/questions/5120/how-do-you-make-samba-follow-symlink-outside-the-shared-path
### http://www.krizna.com/ubuntu/setup-file-server-ubuntu-14-04-samba/

    1. Anonymous share.
    2. Secured share.
    3. User creation.

    smbpasswd -a wood
    smbpasswd -e wood
    smbpasswd -d wood
    smbpasswd -x wood

https://www.samba.org/samba/docs/using_samba/ch09.html

### http://askubuntu.com/questions/153893/samba-and-user-acount-passwords
### https://ubuntuforums.org/showthread.php?t=1996277

    apt-get install samba libpam-smbpass
    pdbedit -L -v

### https://wiki.archlinux.org/index.php/samba

    $ smbclient -L 192.168.1.155 -U%
    $ smbclient -c 'ls' //localhost/public -U wood

### mount

    $ sudo mount -t cifs //192.168.1.11/sales /mnt/cifs -o username=shareuser,password=sharepassword,domain=nixcraft
    $ ##,uid=username,gid=usergroup
    $ sudo mount.cifs //192.168.1.155/smb cifs -o username=administrator,uid=1000

### ubuntu

    sudo service smbd restart

### hide files

https://www.samba.org/samba/docs/using_samba/ch08.html

    veto files = /.*/*.bmp/*.jpg/*.png/*.gif/



### troubles shoot

    ls -ld /
    stat /

https://www.linuxquestions.org/questions/linux-server-73/samba-fails-failed-to-create-pipe-directory-4175664191/

    ExecStartPre=/bin/mkdir /run/samba


https://forums.centos.org/viewtopic.php?t=65876

    systemd-tmpfiles --create /usr/lib/tmpfiles.d/samba.conf

