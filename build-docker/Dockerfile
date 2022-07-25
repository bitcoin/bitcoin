FROM alpine as bitcoin-build

# build depndencies for bitcoind
RUN apk update && apk --no-cache add \
  autoconf \
  automake \
  boost-dev \
  build-base \
  db-c++ \
  db-dev \
  file \
  g++ \
  libevent-dev \
  libressl \
  libtool \
  make

# These dependencies are needed at runtime
# Even though this is alpine we are not yet producing a statically linked output
RUN apk --no-cache add \
  boost-filesystem \
  boost-system \
  boost-thread \
  libevent \
  libzmq

# Not required for building bitcoind but needed by some development scripts
RUN apk --no-cache add bash git py3-pip py3-flake8 py3-mypy shellcheck
RUN pip3 install vulture codespell

# py3-zmq is only available on alpine:edge
RUN apk update && \
  apk add build-base libzmq musl-dev python3 python3-dev zeromq-dev && \
  pip3 install pyzmq && \
  apk del build-base musl-dev python3-dev zeromq-dev

COPY group /etc/group
COPY passwd /etc/passwd
