#!/bin/bash -ev

sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install -y build-essential libssl-dev libboost-all-dev libdb++-dev
sudo apt-get install -y miniupnpc libminiupnpc-dev

sudo timedatectl set-ntp on

cd /vagrant/src
make -f makefile.unix
