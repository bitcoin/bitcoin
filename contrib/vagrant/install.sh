#!/bin/bash -ev

mkdir -p ~/.peercoin
echo "rpcuser=username" >>~/.peercoin/peercoin.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.peercoin/peercoin.conf

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp no
sudo apt-get -y -qq install ntp
sudo ntpq -p

pushd /vagrant
./scripts/dependencies-ubuntu.sh

./scripts/install-ubuntu.sh
popd

