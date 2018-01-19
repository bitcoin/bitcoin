#!/bin/bash -ev

sudo apt-get install -y -qq htop
sudo timedatectl set-ntp no
sudo apt-get -y -qq install ntp
sudo ntpq -p

./scripts/peercoinconf.sh

./scripts/dependencies-ubuntu.sh

./scripts/install-ubuntu.sh

