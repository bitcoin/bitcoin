Dynamo Core
===========

Dynamo is built upon the effort of many individuals who, for over a decade, have championed the idea of currency by the people and for the people.  Our team is deeply grateful for their vision and effort.


All things Dynamo can be found at https://www.dynamocoin.org

Building for windows:

Clone using Visual Studio 2019 community and build.  There are package dependencies which can be most easily installed using VCPKG.  If you build the QT client you will also need runtime libs.  Alot more docs are needed on the build process - please connect with us on Discord for help if you want to build core.

Building for Ubuntu:

clone the repo

sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3
sudo apt-get install libevent-dev libboost-dev libboost-system-dev libboost-filesystem-dev libboost-test-dev

./autogen.sh
./configure --disable-wallet
make

You will get errors on some of the tools as not all are yet supported.  The CLI and core will build.  More docs are needed here, connect with us on Discord for help.
