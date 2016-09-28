#!/bin/bash

cwd="`basename $(pwd)`"
cd ..

find $cwd -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
    -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
    |cpio -o |gzip -c > $cwd.cpio.gz-`date +%F`

echo "gzip -dc $cwd.cpio.gz-`date +%F` |cpio -i"

