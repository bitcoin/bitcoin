#!/bin/bash -ev

mkdir -p ~/.peercoin
echo "rpcuser=username" >>~/.peercoin/peercoin.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.peercoin/peercoin.conf

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp no
sudo apt-get -y -qq install ntp
sudo ntpq -p

pushd /vagrant
./contrib/vagrant/dependencies-ubuntu.sh

./contrib/install_db4.sh `pwd`
#flags arent being picked up, so need to link
sudo ln -sf `pwd`/db4/include /usr/local/include/bdb4.8
sudo ln -sf `pwd`/db4/lib/*.a /usr/local/lib
./autogen.sh
./configure --with-gui=qt5
make -j$(nproc)
make check

popd

