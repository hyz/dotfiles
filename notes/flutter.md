
    pacman -S jdk8-openjdk unzip

    flutter help
    flutter -v upgrade
    flutter -v doctor
    flutter channel
    flutter config

https://flutter.dev/community/china

https://flutterchina.club/get-started/install/

    D:\Android\Sdk\tools\
    bin\avdmanager create avd -n test -k "system-images;android-28;default;x86" -b x86 -c 256M -d 7 -f

https://flutter.dev/community/china

    export CHROME_EXECUTABLE=`which chromium`

    flutter run -d chrome --web-port 8888  --web-hostname 0.0.0.0

    flutter build web \
        --release \
        --web-renderer=canvaskit \ --dart-define=FLUTTER_WEB_CANVASKIT_URL=http://*****//canvaskitSdk/

    flutter run -d chrome # --web-renderer=canvaskit


flutter build web release
https://dart.dev/tools/webdev#build
