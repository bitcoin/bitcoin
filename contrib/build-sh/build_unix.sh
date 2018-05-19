#!/bin/sh
echo -ne '\n' | sudo add-apt-repository ppa:bitcoin/bitcoin
sudo apt-get update -y
sudo apt-get install -y build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils autoconf
sudo apt-get install -y libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev
sudo apt-get install -y libboost-all-dev
sudo apt-get install -y libdb4.8-dev libdb4.8++-dev
sudo apt-get install -y libminiupnpc-dev
sudo apt-get install -y libzmq3-dev
sudo apt-get install -y libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
sudo apt-get install -y libqrencode-dev
cd ../..
echo '#################################################'
echo '# Building DB 4.8.30 NC                         #'
echo '#################################################'
AIT_ROOT=$(pwd)
BDB_PREFIX="${AIT_ROOT}/db4"
mkdir -p $BDB_PREFIX
wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
echo '12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef  db-4.8.30.NC.tar.gz' | sha256sum -c
tar -xzvf db-4.8.30.NC.tar.gz
cd db-4.8.30.NC/build_unix/
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX
make install
echo '#################################################'
echo '# Building Aither Core                          #'
echo '#################################################'
cd $AIT_ROOT
chmod ugo+x autogen.sh
./autogen.sh
./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/"
make
mkdir -p release
cp src/aitherd $AIT_ROOT/release
cp src/aither-tx $AIT_ROOT/release
cp src/aither-cli $AIT_ROOT/release
cp src/qt/aither-qt $AIT_ROOT/release
