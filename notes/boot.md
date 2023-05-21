
    efibootmgr --create -b 7 --index 1 --disk /dev/nvme0n1 --part 1 --write-signature --loader /EFI/ubuntu/shimx64.efi --label "Ubuntu"

### security boot

    https://unix.stackexchange.com/questions/234129/sign-grub2-bootloader-to-enable-uefi-secure-boot
    https://wiki.gentoo.org/wiki/Sakaki%27s_EFI_Install_Guide/Configuring_Secure_Boot

- backed up the default keys from MS:

    #efi-readvar -v PK -o old_PK.esl
    #efi-readvar -v KEK -o old_KEK.esl
    #efi-readvar -v db -o old_db.esl
    #efi-readvar -v dbx -o old_dbx.esl

- generated my own keys:

    #openssl req -new -x509 -newkey rsa:2048 -days 3650 -nodes -sha256 -subj "/CN=my platform key/" -keyout PK.key -out PK.crt
    #openssl req -new -x509 -newkey rsa:2048 -days 3650 -nodes -sha256 -subj "/CN=my key exchange key/" -keyout KEK.key -out KEK.crt
    #openssl req -new -x509 -newkey rsa:2048 -days 3650 -nodes -sha256 -subj "/CN=my kernel signing key/" -keyout db.key -out db.crt
    #cert-to-efi-sig-list -g "$(uuidgen)" PK.crt PK.esl
    #sign-efi-sig-list -k PK.key -c PK.crt PK PK.esl PK.auth

- cleared the keys in BIOS so efi-readvar outputs no keys on reboot

- set the backed up keys back and appended mine:

    #efi-updatevar -e -f old_KEK.esl KEK
    #efi-updatevar -e -f old_db.esl db
    #efi-updatevar -e -f old_dbx.esl dbx
    #efi-updatevar -a -c KEK.crt KEK
    #efi-updatevar -a -c db.crt db
    #efi-updatevar -f PK.auth PK

- signed the bootloader:

    #cp /boot/EFI/grub/grubx64.efi /boot/EFI/grub/grubx64.efi.old
    #sbsign --key db.key --cert db.crt --output /boot/EFI/grub/grubx64.efi /boot/EFI/grub/grubx64.efi

The verification runs through:

#sbverify --cert db.crt /boot/EFI/grub/grubx64.efi
 Signature verification OK.

