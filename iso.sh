#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/projectos.kernel isodir/boot/projectos.kernel
cat >isodir/boot/grub/grub.cfg <<EOF
menuentry "projectos" {
	multiboot /boot/projectos.kernel
}
EOF
grub-mkrescue -o ProjectOS.iso isodir
