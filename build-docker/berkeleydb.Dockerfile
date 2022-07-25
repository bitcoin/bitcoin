FROM alpine as bitcoin-build-db

# build depndencies for Berkeley DB
RUN apk update && apk --no-cache add \
  autoconf \
  automake \
  build-base \
  libressl \
  patch

ENV BERKELEYDB_VERSION=db-4.8.30.NC
