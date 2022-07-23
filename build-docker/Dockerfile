FROM alpine

# build depndencies
RUN apk update && apk add \
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

# Not required for building bitcoind but needed by some scripts
RUN apk add bash py3-pip shellcheck py3-flake8 py3-mypy
RUN pip3 install vulture codespell

# py3-zmq is available on alpine:edge
RUN apk update && \
  apk add build-base libzmq musl-dev python3 python3-dev zeromq-dev && \
  pip3 install pyzmq && \
  apk del build-base musl-dev python3-dev zeromq-dev

COPY group /etc/group
COPY passwd /etc/passwd
