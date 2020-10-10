sudo add-apt-repository ppa:bitcoin/bitcoin
sudo apt-get update

# Bitcoin dependencies
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
# Build with self-compiled depends to re-enable wallet compatibility
sudo apt-get install libssl-dev libevent-dev libboost-all-dev

# GUI dependencies
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
sudo apt-get install libqrencode-dev

# Bitcoin wallet functionality
sudo apt-get install libdb-dev libdb++-dev

# Incoming connections
sudo apt-get install libminiupnpc-dev

# Used to check what windows are open
sudo apt-get install wmctrl

./autogen.sh
./configure --with-miniupnpc --enable-upnp-default --with-incompatible-bdb # --disablewallet # --prefix=`pwd`/depends/x86_64-linux-gnu
#./configure --without-miniupnpc --enable-upnp-default --with-incompatible-bdb # --disablewallet # --prefix=`pwd`/depends/x86_64-linux-gnu
make -j8
./run.sh
