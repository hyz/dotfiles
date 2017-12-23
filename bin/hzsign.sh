#!/bin/sh
# hzsign.sh src/Game16

#export ANDROID_SERIAL=PJGUAQFY99999999

#adb shell mount -t rootfs -o remount,rw rootfs /system 

### "C:\\Program Files\\Java\\jdk1.7.0_21\\bin\\java"

#java -jar signapk.jar platform.x509.pem platform.pk8 BarcodeNewUI.apk Game.apk 

[ -n "$1" ] || exit 1

Src=$1
SignDir=$HOME/hzmake/sign

rm -vf $SignDir/{libmtkhw.so,libBarcode.so,Game.apk}

/bin/cp -v $Src/libs/armeabi-v7a/libBarcode.so $SignDir/ || exit 2
/bin/cp -v $Src/libmtkhw.so $SignDir/ || exit 3
/bin/ls -l $SignDir/Game-Unsigned.apk || exit 4
#echo "" ; read y
#cp -v $Src/libBarcode.so $SignDir/
#cp -v $Src/libmtkhw.so $SignDir/
#cp -v $Src/Game.apk $SignDir/Game-Unsigned.apk

cd $SignDir
java -jar signapk.jar platform.x509.pem platform.pk8 Game-Unsigned.apk Game.apk 

adb -d shell mount -o rw,remount,rw /system

adb -d shell rm -vf /system/app/Game/Game.apk /system/app/Game.apk
for x in libBarcode.so libmtkhw.so ; do
    adb -d shell rm -vf /system/app/Game/$x /system/lib/$x /system/app/$x
done
#adb -d shell rm -f /system/app/Game/Game.apk /system/app/Game.apk
#adb -d shell rm -f /system/lib/libBarcode.so
#adb -d shell rm -f /system/lib/libmtkhw.so

adb -d push libmtkhw.so     /system/lib/
adb -d push libBarcode.so   /system/lib/
adb -d push Game.apk        /system/app/

#adb -d shell setenforce 0
#adb -d shell chmod 0666 /dev/Vcodec /dev/MTK_SMI /sys/bus/platform/drivers/mem_bw_ctrl/concurrency_scenario

# adb -d shell reboot

