
#export PATH=$PATH:/home/wood/mt6580/alps/out/host/linux-x86/bin
#export PATH=$PATH:`pwd`/bin
export PATH=$PATH:$HOME/bin

die() {
    echo $* ; exit 1 ;
}

which simg2img make_ext4fs || die "which"

mkdir TMP MNT

simg2img system.img TMP/system.img.ext4 || die "simg2img"
mount -t ext4 -o loop TMP/system.img.ext4 MNT || die "mount"

find MNT -name Game.apk -o -name libmtkhw.so -o -name libBarcode.so

mv system.img bak.system.img-`date +%H%M`
make_ext4fs -s -l 512M -a system system.img MNT

umount MNT
rm -rf TMP MNT

