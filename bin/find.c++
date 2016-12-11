#!/bin/bash

find -P $* -type f \( \
    -name "*.h" -o -name "*.c" \
    -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \
    -o -name "*.mk" -o -name "*.jam" -o -name "[Mm]akefile" \
  \) # |cpio -o |gzip -c

