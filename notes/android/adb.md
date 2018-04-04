
### https://gist.github.com/Pulimet/5013acf2cd5b28e55036c82c91bd56d8

Adb useful commands list

### https://stackoverflow.com/questions/4032960/how-do-i-get-an-apk-file-from-an-android-device

    adb shell pm list packages |grep com.xxx.yyy
    adb shell pm path com.xxx.yyy
    adb pull /system/app/Game.apk

### cygwin

    export SDK=/cygdrive/d/Android/sdk ; export PATH=$PATH:$SDK/platform-tools:$SDK/tools

###

    adb push a.h264 /sdcard/
    adb shell ls /sdcard

    adb shell pm list permissions -s
    adb shell pm list permissions
    adb shell pm list permissions -g
    adb shell dumpsys package com.foo.bar |egrep -A99999 "grantedPermissions:"

    adb logcat -s '<TAG>'

    adb -d shell kill -9 `adb -d shell ps |grep com.example.a |awk '{print $2}'`

###

    adb -s HU9SP7ZLSO5DNNEA push bin/bw /data
    adb -s HU9SP7ZLSO5DNNEA shell

###

    xlog filter-set verbose

http://www.jianshu.com/p/296faefe3226

    adb shell /system/bin/screencap -p /sdcard/screenshot.png
        adb pull /sdcard/screenshot.png .

### pm

    adb shell pm list packages com.example
    adb shell pm list packages |grep -v com.and |grep -v com.media

### am

    adb shell am ## --help
    adb shell am start -n com.example.hello/.MainActivity
    adb shell monkey -p com.example.hello 1
    adb shell am start -a android.intent.action.MAIN -n com.android.settings/.wifi.WifiSettings
    adb shell am start -a android.intent.action.MAIN -n com.huazhen.barcode/.app.LogoActivity

### broadcast

http://stackoverflow.com/questions/5171354/android-adb-shell-am-broadcast-bad-component-name

    adb shell am broadcast android.net.conn.CONNECTIVITY_CHANGE
        Broadcasting: Intent { act=android.intent.action.VIEW dat=android.net.conn.CONNECTIVITY_CHANGE }
        Broadcast completed: result=0
    adb shell am broadcast android.intent.action.BOOT_COMPLETED
        Broadcasting: Intent { act=android.intent.action.VIEW dat=android.intent.action.BOOT_COMPLETED }
        Broadcast completed: result=0

http://blog.csdn.net/sunrock/article/details/5675067
https://blog.csdn.net/soslinken/article/details/50245865

    adb shell am start -a android.intent.action.VIEW -d http://g.cn
        ##java
        # Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://g.cn"));
        # startActivity(browserIntent);
    adb shell am start -a android.intent.action.CALL -d tel:10086
    adb shell service call phone 1 s16 "10086"
    adb shell am start -a android.intent.action.VIEW -d 'geo:0,0?q=shanghai'

    adb shell am start -a android.settings.INPUT_METHOD_SETTINGS

### input

    adb shell input text 'http://192.168.2.115:8000'

    ## http://stackoverflow.com/questions/7789826/adb-shell-input-events
    ## https://zhuanlan.zhihu.com/p/26236061
    ##  4 -->  "KEYCODE_BACK" 
    ##  3 -->  "KEYCODE_HOME" 
    ##  5 -->  "KEYCODE_CALL" 
    ## 24 -->  "KEYCODE_VOLUME_UP" 
    ## 25 -->  "KEYCODE_VOLUME_DOWN" 
    ## 26 -->  "KEYCODE_POWER" 
    ## 27 -->  "KEYCODE_CAMERA" 
    adb shell input keyevent 4
    adb shell input keyevent 3
    adb shell input --longpress keyevent 3

### svc

    adb shell svc wifi disable                                                 ~
    adb shell svc wifi enable                                                  ~

### http://stackoverflow.com/questions/4567904/how-to-start-an-application-using-android-adb-tools?rq=1
### http://stackoverflow.com/questions/17829606/android-adb-stop-application-command-like-force-stop-for-non-rooted-device

    #stop/kill
    adb shell am force-stop com.example.hello

adb.exe programming:

让Adb.exe支持Monkey

http://www.jianshu.com/p/d0e7f19302c9
http://osxr.org/android/source/system/core/adb/
https://github.com/t-mat/my_adb

### http://stackoverflow.com/questions/4032960/how-do-i-get-an-apk-file-from-an-android-device?rq=1

How do I get an apk file from an Android device?

    adb shell pm list packages
    adb shell pm path com.android.wallpaper 
    adb pull /system/app/LiveWallpapers/LiveWallpapers.apk .

    adb backup -apk com.twitter.android
    dd if=backup.ab bs=24 skip=1 | openssl zlib -d > backup.tar

### http://stackoverflow.com/questions/2604727/how-can-i-connect-to-android-with-adb-over-tcp
### http://stackoverflow.com/questions/4893953/run-install-debug-android-applications-over-wi-fi

    su
    setprop service.adb.tcp.port 5555
    stop adbd
    start adbd

    adb shell netcfg
    adb tcpip 5555
    adb connect 192.168.0.101:5555

