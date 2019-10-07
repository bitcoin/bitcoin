#!/usr/bin/env sh

set -ex

mkdir -m 777 /qemu
cd /qemu

curl --retry 5 -LO https://cdimage.debian.org/cdimage/ports/10.0/sparc64/iso-cd/debian-10.0-sparc64-NETINST-1.iso
7z e debian-10.0-sparc64-NETINST-1.iso boot/initrd.gz
7z e debian-10.0-sparc64-NETINST-1.iso boot/sparc64
mv sparc64 kernel
rm debian-10.0-sparc64-NETINST-1.iso

mkdir init
cd init
gunzip -c ../initrd.gz | cpio -id
rm ../initrd.gz
cp /usr/sparc64-linux-gnu/lib/libgcc_s.so.1 usr/lib/
chmod a+w .
