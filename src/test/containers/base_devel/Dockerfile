FROM ubuntu:14.04
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8842CE5E
RUN echo deb http://ppa.launchpad.net/bitcoin/bitcoin/ubuntu trusty main >/etc/apt/sources.list.d/bitcoin.list
RUN apt-get update
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get install -y git build-essential g++
RUN apt-get install -y libboost-all-dev libdb4.8++-dev libqrencode-dev
RUN apt-get install -y libssl-dev
