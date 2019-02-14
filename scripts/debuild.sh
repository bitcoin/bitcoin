#!/bin/bash -ev

#debuild and associated tools
sudo apt-get install -qqy devscripts debhelper

#not sure why these are only needed when packaging and not for the compile
sudo apt-get install -qqy bash-completion libevent-dev qttools5-dev libqrencode-dev

sudo apt-get install -qqy libzmq3-dev
#need to replace by script so not incompatible bdb
#sudo apt-get install -qqy libdb++-dev
./contrib/install_db4.sh `pwd`
#flags arent being picked up, so need to link
sudo ln -sf `pwd`/db4/include /usr/local/include/bdb4.8
sudo ln -sf `pwd`/db4/lib/*.a /usr/local/lib

export DEB_BUILD_OPTIONS="parallel=$(nproc)"

rm -rf debian
cp -r contrib/debian .

#binary build for now
debuild -b -us -uc

