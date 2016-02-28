#!/bin/bash

#  Shell script that converts gitian built binaries into a simple DPKG, which can then be put into an apt repo.
#
#  Improvement ideas:
#  - Install the man page from the source repo.
#  - Wrap in a script that sends crash reports/core dumps to some issue tracker.
#  - etc ...

ver=0.12
realver=0.12

set +e

# Make working space
workdir=bitcoincl-$realver
[ -d $workdir ] && rm -r $workdir
mkdir $workdir
cd $workdir

# Extract the tarball to a directory called usr
tarball=bitcoin-$ver-linux64.tar.gz
tar xzvf ../$tarball
mv bitcoin-$ver usr

# copy bitcoincld.service file to lib/systemd/system directory
mkdir -p lib/systemd/system 
cp ../bitcoincld.service lib/systemd/system

# copy bitcoin.conf file to etc/bitcoincl
mkdir -p etc/bitcoincl
cp ../bitcoin.conf etc/bitcoincl

# create file to force creation of data folder
mkdir -p var/lib/bitcoincl
touch var/lib/bitcoincl/.empty

# Rename the binaries so we don't conflict with regular Bitcoin
mv usr/bin/bitcoind usr/bin/bitcoincld
mv usr/bin/bitcoin-cli usr/bin/bitcoincl-cli
mv usr/bin/bitcoin-tx usr/bin/bitcoincl-tx
mv usr/bin/bitcoin-qt usr/bin/bitcoincl-qt

# Remove unneeded files 
rm usr/bin/test_bitcoin
rm usr/bin/test_bitcoin-qt
rm usr/include/*
rm usr/lib/*

# Set up debian metadata. There are no dependencies beyond libc and other base DSOs as everything is statically linked.

mkdir DEBIAN
cat <<EOF >DEBIAN/control
Package: bitcoincl
Architecture: amd64
Description: Bitcoin Classic is a fully verifying Bitcoin node implementation based on the sources of Bitcoin Core.
Maintainer: Steve Myers <steven.myers@gmail.com>
Version: $realver
Depends: debconf, adduser, ntp
EOF

cat <<EOF >DEBIAN/install
usr/bin/bitcoincld usr/bin
usr/bin/bitcoincl-cli usr/bin
usr/bin/bitcoincl-tx usr/bin
EOF

cat <<EOF >DEBIAN/conffiles
lib/systemd/system/bitcoincld.service
etc/bitcoincl/bitcoin.conf
var/lib/bitcoincl/.empty
EOF

# copy templates file to DEBIAN/templates
cp ../templates DEBIAN/templates

# copy the postinst file to DEBIAN/postinst
cp ../postinst DEBIAN/postinst
chmod 0755 DEBIAN/postinst 

# copy the prerm file to DEBIAN/prerm
cp ../prerm DEBIAN/prerm
chmod 0755 DEBIAN/prerm 

# copy the postrm file to DEBIAN/postrm
cp ../postrm DEBIAN/postrm
chmod 0755 DEBIAN/postrm

cd ..

# Build deb
dpkg-deb --build $workdir
