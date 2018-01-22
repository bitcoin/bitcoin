
### Fedora 27 Instructions

The following are the steps I did on fedora 27 to compile the wallet:
```
yum -y install automake autoconf gcc-c++ protobuf-devel qrencode-devel libevent-devel
yum -y install libdb4-devel libdb4-cxx-devel qt5-devel miniupnpc-devel
yum -y install boost-devel libtool 
yum -y install compat-openssl0-devel.x86_64      <--- Very important !!! otherwise you get configure error libressl
```
Note: Do not use the built in SSL library, v1.1.x is not supported.
```
git clone https://github.com/Hodlcoin/Hodlcoin.git
cd Hodlcoin
./autogen.sh
./configure --enable-tests=no --enable-cxx --with-tls
make -j2 
```

You should have a working wallet with gui at this point.
