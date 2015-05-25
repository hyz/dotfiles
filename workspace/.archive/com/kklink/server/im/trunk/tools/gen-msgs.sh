#!/bin/bash

## 1
# redis-cli <<<"hgetall users" | python xtools.py list_uidtok >_0

## 2
cat _0 | python xtools.py rand205 2 3 \
    | tee >(awk '$2==1{print}' >_1) \
    | tee >(awk '$2==99{print}' >_2) \
    | awk '$2==205{print}' >_3

## 3
# yxim-test --ack=1 -h 127.0.0.1 -p 8443 _1 _2 2>err.log # >out.log

## 4
# grep End err.log
# grep End err.log |awk '{print $2}' |while read x ; do sed -i "/^$x/d" _0 ; done
#
# goto #2

## 5
# yxim-test --ack=1 -h 127.0.0.1 -p 8443 _1 _2 _3 2>err.log >out.log
#

