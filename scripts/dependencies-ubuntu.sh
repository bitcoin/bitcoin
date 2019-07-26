#!/bin/bash -ev

sudo apt-get update -qq
sudo apt-get upgrade -y -qq
sudo apt-get install -y -qq autoconf build-essential pkg-config libssl-dev libboost-all-dev
sudo apt-get install -y -qq miniupnpc libminiupnpc-dev gettext libevent-dev protobuf-compiler libprotobuf-dev
#for gui
sudo apt-get install -y -qq qtbase5-dev qttools5-dev-tools
sudo apt-get install -y -qq libdb++-dev
#for debuild
sudo apt-get install -y -qq libfakeroot fakeroot
