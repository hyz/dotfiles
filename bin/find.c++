#!/bin/bash

find -P $* \
     -type d \( \
         -path "*/GeneratedFiles" \
     \) -prune \
     -o -type f \( \
         -name "*.h" -o -name "*.c" \
         -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
         -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
         -o -name "*.pro" \
     \) -print \
# |cpio -o |gzip -c

# cpio -o > x.cpio
# cpio -oAF x.cpio

