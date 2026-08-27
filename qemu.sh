#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom ProjectOS.iso -drive file=projectOS_drive.img,format=raw,index=0,media=disk
