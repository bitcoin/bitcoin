#!/usr/bin/env sh

set -ex

mkdir -m 777 /qemu
cd /qemu

curl --retry 5 -LO https://github.com/qemu/qemu/raw/master/pc-bios/s390-ccw.img
curl --retry 5 -LO http://ftp.debian.org/debian/dists/testing/main/installer-s390x/20190410/images/generic/kernel.debian
curl --retry 5 -LO http://ftp.debian.org/debian/dists/testing/main/installer-s390x/20190410/images/generic/initrd.debian

mv kernel.debian kernel
mv initrd.debian initrd.gz

mkdir init
cd init
gunzip -c ../initrd.gz | cpio -id
rm ../initrd.gz
cp /usr/s390x-linux-gnu/lib/libgcc_s.so.1 usr/lib/
chmod a+w .
