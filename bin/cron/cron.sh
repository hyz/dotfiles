#!/bin/bash

#test -n "$DISPLAY" || export DISPLAY=:0
[ -n "$DBUS_SESSION_BUS_ADDRESS" ] || export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus
#test -n "$DBUS_SESSION_BUS_ADDRESS" || eval `dbus-launch --sh-syntax`

#FVWM_PID=$(pgrep fvwm)
#export DBUS_SESSION_BUS_ADDRESS=$(grep -z DBUS_SESSION_BUS_ADDRESS /proc/$FVWM_PID/environ|cut -d= -f2-)

exec > /tmp/cron.sh.log 2>&1
echo

date
env
#$HOME/bin/my-notify &

