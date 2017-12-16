#!/bin/bash -ev

sudo apt-get update -qq
sudo apt-get upgrade -y -qq
sudo apt-get install -y -qq autoconf build-essential pkg-config libssl-dev libboost-all-dev 
sudo apt-get install -y -qq miniupnpc libminiupnpc-dev
#for gui
sudo apt-get install -y -qq qtbase5-dev qttools5-dev-tools 
sudo apt-get install -y -qq libdb++-dev

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp on

sudo apt -y -qq install gettext

mkdir -p ~/.peercoin
echo "rpcuser=username" >>~/.peercoin/peercoin.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.peercoin/peercoin.conf

pushd /vagrant

#install berkleydb 4.8
#popd
#ubuntu specific
#sudo apt-get install software-properties-common
#sudo add-apt-repository -y ppa:bitcoin/bitcoin
#sudo apt-get update -qq
#sudo apt-get install -y libdb4.8-dev libdb4.8++-dev

./contrib/install_db4.sh `pwd`

#flags arent being picked up, so need to link
sudo ln -s /vagrant/db4/include /usr/local/include/bdb4.8
sudo ln -s /vagrant/db4/lib/*.a /usr/local/lib

./autogen.sh
./configure --with-gui=qt5 CPPFLAGS="-I/vagrant/db4/include" LDFLAGS="-L/vagrant/db4/lib"
make -j$(nproc)

popd

