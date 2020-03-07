#!/bin/bash

[ -r "$1" ] || exit 1
echo `/bin/grep -vE '^\s*#' $1`

#QEMU_AUDIO_DRV=pa QEMU_PA_SAMPLES=128 taskset -c 21-31 qemu-system-x86_64 `/bin/grep -vE '^\s*#' $1`
#QEMU_AUDIO_DRV=alsa
