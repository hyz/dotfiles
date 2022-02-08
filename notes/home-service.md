
### media server - gerbera

    > cp -vt .config/systemd/user /usr/lib/systemd/system/gerbera.service
    > vim ...
    > cat .config/systemd/user/gerbera.service
    > systemctl --user enable gerbera

    [Unit]
    Description=Gerbera Media Server
    After=network.target

    [Service]
    Type=simple
    #User=gerbera
    #Group=gerbera
    ExecStart=/usr/bin/gerbera -c .config/gerbera/config.xml
    Restart=on-failure
    RestartSec=5

    [Install]
    WantedBy=multi-user.target


### https://docs.gerbera.io/en/stable/config-import.html

    gerbera --add-file /home/library/Music

    sudo systemctl stop gerbera
    sudo -u gerbera -- /bin/gerbera -c /etc/gerbera/config.xml --add-file ../Medicine医学../...

