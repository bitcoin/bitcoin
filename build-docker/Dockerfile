FROM alpine

# build depndencies
RUN apk update && apk add
  git \
  make \
  file \
  autoconf \
  automake \
  build-base \
  libtool \
  db-c++ \
  db-dev \
  boost-system \
  boost-program_options \
  boost-filesystem \
  boost-dev \
  libressl-dev \
  libevent-dev

# Needed by some scripts
RUN apk add bash

COPY group /etc/group
COPY passwd /etc/passwd
