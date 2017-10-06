FROM ubuntu:16.04
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8842CE5E
RUN echo deb http://ppa.launchpad.net/bitcoin/bitcoin/ubuntu trusty main >/etc/apt/sources.list.d/bitcoin.list
RUN apt-get update
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get install -y libboost-filesystem1.58.0 libboost-program-options1.58.0 libboost-thread1.58.0 libdb4.8++
RUN apt-get install -y libssl1.0.0
