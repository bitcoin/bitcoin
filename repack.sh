#!/bin/bash
cd build/out || exit "run this from the gitian-builder directory"
mkdir -p tmp
cd tmp
rm -rf bitcoin-0.11.2/ bitcoinUnlimited-0.11.2/
tar xvfz ../bitcoin-0.11.2-linux64.tar.gz
mv bitcoin-0.11.2/ bitcoinUnlimited-0.11.2/
find bitcoinUnlimited-0.11.2 | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ../bitcoinUnlimited-0.11.2-linux64.tar.gz

rm -rf bitcoinUnlimited-0.11.2/
tar xvfz ../bitcoin-0.11.2-linux32.tar.gz
mv bitcoin-0.11.2/ bitcoinUnlimited-0.11.2/
find bitcoinUnlimited-0.11.2 | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ../bitcoinUnlimited-0.11.2-linux32.tar.gz

rm -rf bitcoin-0.11.2/ bitcoinUnlimited-0.11.2/

cd ../../..
