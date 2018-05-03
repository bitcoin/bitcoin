FROM debian:stretch
LABEL maintainer="Syscoin Developers <jsidhu@blockchainfoundry.co>"
LABEL description="Dockerised SyscoinCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
