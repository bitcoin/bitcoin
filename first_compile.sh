sudo add-apt-repository ppa:bitcoin/bitcoin
sudo apt-get update

# Bitcoin dependencies
sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
# Build with self-compiled depends to re-enable wallet compatibility
sudo apt-get install libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev

# GUI dependencies
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
sudo apt-get install libqrencode-dev

# Bitcoin wallet functionality
sudo apt-get install libdb4.8-dev libdb4.8++-dev

./autogen.sh
./configure --enable-upnp-default # --prefix=`pwd`/depends/x86_64-linux-gnu
make -j8
./run.sh
