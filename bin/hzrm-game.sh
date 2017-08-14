#!/bin/sh

# SDK=/cygdrive/d/Android/sdk
# export PATH=$PATH:$SDK/platform-tools:$SDK/tools

adb -d shell mount -o rw,remount,rw /system # adb -d remount
adb -d shell "mount -t rootfs -o remount,rw rootfs /system"

for x in Game.apk libBarcode.so libmtkhw.so ; do
    adb -d shell rm -f /system/app/$x
    adb -d shell rm -f /system/lib/$x
    adb -d shell rm -f /system/app/Game/$x
done

adb -d shell reboot

