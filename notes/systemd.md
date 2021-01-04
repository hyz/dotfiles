
### cat /lib/systemd/system/pyxls-grep.service

    [Unit]
    Description=pyxls-grep
    Documentation=
    After=network.target

    [Service]
    Type=simple
    #LimitNOFILE=32768
    Environment="LANG=en_US.UTF-8"
    User=wood
    WorkingDirectory=/home/wood/www
    ExecStart=/home/wood/bin/pyxls-grep

    [Install]
    WantedBy=multi-user.target

### cat bin/pyxls-grep

    #!/bin/bash

    env > /tmp/1.log
    exec python3 /home/wood/bin/pyxls-grep.py # 2>/tmp/1.log

### https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux

New-Style Daemons
https://www.freedesktop.org/software/systemd/man/daemon.html#New-Style%20Daemons

https://github.com/jirihnidek/daemon

### https://www.freedesktop.org/wiki/Software/systemd/FrequentlyAskedQuestions/

### https://superuser.com/questions/278396/systemd-does-not-run-etc-rc-local

/etc/rc.local

### https://medium.com/horrible-hacks/using-systemd-as-a-better-cron-a4023eea996d

Using systemd as a better cron

/etc/systemd/system/shopify-recent.service

    [Unit]
    Description=Runs shopify recent scraper
    Wants=shopify-recent.timer

    [Service]
    ExecStart=/usr/local/bin/node /app/shoes-scraper/src/scraper --recent --platform shopify
    WorkingDirectory=/app/shoes-scraper
    Slice=shoes-scraper.slice

    [Install]
    WantedBy=multi-user.target

/etc/systemd/system/shopify-recent.timer

    [Unit]
    Description=Run shopify-recent every 15-30 minutes
    Requires=shopify-recent.service

    [Timer]
    Unit=shopify-recent.service
    OnUnitInactiveSec=15m
    RandomizedDelaySec=15m
    AccuracySec=1s

    [Install]
    WantedBy=timers.target

/etc/systemd/system/shoes-scraper.slice

    [Unit]
    Description=Limited resources Slice
    DefaultDependencies=no
    Before=slices.target

    [Slice]
    CPUQuota=80%
    MemoryLimit=2.7G

Handy commands

    systemctl list-timers  
    journalctl -f -u shopify-recent.service

https://unix.stackexchange.com/questions/278564/cron-vs-systemd-timers
https://jason.the-graham.com/2013/03/06/how-to-use-systemd-timers/


https://wiki.archlinux.org/index.php/Systemd-boot


### cat /lib/systemd/system/sss.service

    [Unit]
    Description=sss
    Documentation=
    After=network.target

    [Service]
    Type=simple
    Environment="LANG=en_US.UTF-8"
    #LimitNOFILE=32768
    #User=wood
    #WorkingDirectory=/home/wood
    ExecStart=/usr/local/bin/sss -c /etc/sss-config.json

    [Install]
    WantedBy=multi-user.target


### cat /lib/systemd/system/kcps.service 

    [Unit]
    Description=kcps
    Documentation=
    After=network.target

    [Service]
    Type=simple
    Environment="LANG=en_US.UTF-8"
    #LimitNOFILE=32768
    #User=wood
    #WorkingDirectory=/home/wood
    ExecStart=/usr/local/bin/kcps -c /etc/kcps.json

    [Install]
    WantedBy=multi-user.target

### cat /etc/kcps.json 
    {
        "listen": ":18908",
        "target": "127.0.0.1:31974",
        "key": "xxxxxxxxxxxxxxxxx",
        "crypt": "aes-128",
        "mode": "fast2",
        "mtu": 1350,
        "sndwnd": 1024,
        "rcvwnd": 1024,
        "datashard": 70,
        "parityshard": 30,
        "dscp": 46,
        "nocomp": true,
        "acknodelay": false,
        "nodelay": 0,
        "interval": 40,
        "resend": 0,
        "nc": 0,
        "sockbuf": 4194304,
        "keepalive": 10
    }

### cat /etc/sss-config.json 
    {
        "server":"127.0.0.1",
        "server_port":31974,
        "local_address":"127.0.0.1",
        "local_port":31974,
        "password":"xxxxxxxxxxxxxxxxxxx",
        "timeout":600,
        "method":"aes-256-cfb"
    }

