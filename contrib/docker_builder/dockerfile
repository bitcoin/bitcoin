FROM ubuntu:latest

# Install deps
RUN apt-get update -y              \
 &&  apt-get install -y             \
  qt4-qmake                    \
  libqt4-dev                   \
  build-essential              \
  libboost-dev                 \
  libboost-system-dev          \
  libboost-filesystem-dev      \
  libboost-program-options-dev \ 
  libboost-thread-dev          \
  libssl-dev                   \
  libdb++-dev                  \
  libqrencode-dev              \
  wget                         \
  pkg-config                   \
  libpng3-dev
                         

RUN wget http://fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz; tar zxf ./qrencode-3.4.4.tar.gz 
WORKDIR /qrencode-3.4.4
RUN ./configure --enable-static; \ 
  make; \
  make install

VOLUME /novacoin

WORKDIR /novacoin

ENTRYPOINT qmake USE_O3=1 USE_ASM=1 RELEASE=1 && \
 make && \
 cd src && \
 make -f makefile.unix USE_O3=1 USE_ASM=1 STATIC=1 && \
 strip novacoind

 
