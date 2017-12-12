#!/bin/bash -ev

sudo apt-get update -qq
sudo apt-get upgrade -y -qq
sudo apt-get install -y -qq autoconf build-essential pkg-config libssl-dev libboost-all-dev libdb++-dev
sudo apt-get install -y -qq miniupnpc libminiupnpc-dev

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp on

sudo apt-get -y install build-essential libqt4-dev qt5-qmake cmake qttools5-dev libqt5webkit5-dev qttools5-dev-tools qt5-default python-sphinx texlive-latex-base inotify-tools libboost-dev libboost-system-dev libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev openssl libssl-dev libdb++-dev libminiupnpc-dev git sqlite3 libsqlite3-dev g++ libpng-dev

mkdir -p ~/.peercoin
echo "rpcuser=username" >>~/.peercoin/peercoin.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.peercoin/peercoin.conf

#    git clone https://github.com/peercoin/peercoin
#    cd peercoin
#    git checkout v0.6.1ppc
#    qmake -qt=qt5 && make


pushd /vagrant
git checkout v0.6.1ppc

pushd src
qmake -qt=qt5 && make -j$(nproc)
make -f makefile.unix -j$(nproc)

#install berkleydb 4.8
#pushd contrib
#./install_db4.sh `pwd`
#popd


#./autogen.sh
#export BDB_PREFIX='/vagrant/contrib/db4'
#./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/"

#./configure --with-incompatible-bdb
#make -j 8

popd

