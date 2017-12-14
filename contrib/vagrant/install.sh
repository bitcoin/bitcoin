#!/bin/bash -ev

sudo apt-get update -qq
sudo apt-get upgrade -y -qq
sudo apt-get install -y -qq autoconf build-essential pkg-config libssl-dev libboost-all-dev #libdb++-dev
sudo apt-get install -y -qq miniupnpc libminiupnpc-dev

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp on

sudo apt-get -y install build-essential libqt4-dev qt5-qmake cmake qttools5-dev libqt5webkit5-dev qttools5-dev-tools qt5-default python-sphinx texlive-latex-base inotify-tools libboost-dev libboost-system-dev libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev openssl libssl-dev libminiupnpc-dev git sqlite3 libsqlite3-dev g++ libpng-dev

sudo apt -y install gettext

mkdir -p ~/.peercoin
echo "rpcuser=username" >>~/.peercoin/peercoin.conf
echo "rpcpassword=`head -c 32 /dev/urandom | base64`" >>~/.peercoin/peercoin.conf

pushd /vagrant

#install berkleydb 4.8
#pushd contrib
#./install_db4.sh `pwd`
#popd
#ubuntu specific for now
sudo apt-get install software-properties-common
sudo add-apt-repository -y ppa:bitcoin/bitcoin
sudo apt-get update -qq
sudo apt-get install -y libdb4.8-dev libdb4.8++-dev


./autogen.sh
export BDB_PREFIX='/vagrant/contrib/db4'
./configure LDFLAGS="$LDFLAGS -L${BDB_PREFIX}/lib/" CPPFLAGS="$CPPFLAGS -I${BDB_PREFIX}/include/" 
make

popd

