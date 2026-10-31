#!/bin/sh

dd if=/dev/zero of=projectOS_drive.img bs=1M count=64
mkfs.ext2 -b 1024 -F projectOS_drive.img
