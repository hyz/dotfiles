#!/bin/sh

ulimit -c unlimited
cd ${1:-.}

YXPID=`cat /tmp/pid.yx.9000`
while true; do
  if [ -f "/proc/$YXPID/status" ]; then
      sleep 16
  else
      usr/local/bin/moon -d -c
      sleep 6
      YXPID=`cat /tmp/pid.yx.9000`
  fi
done

