
    export PULSE_SERVER=tcp:127.0.0.1
    mplayer ...

### .config/pulse/default.pa

    load-module module-native-protocol-tcp auth-ip-acl=127.0.0.1;192.168.1.0/16

### ---

    PULSE_SERVER=192.168.1.24 mplayer some-movie.avi
    PULSE_SERVER=tcp:192.168.1.24 mplayer -vo null ...

