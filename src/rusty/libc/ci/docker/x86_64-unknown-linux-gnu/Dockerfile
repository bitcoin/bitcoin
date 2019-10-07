FROM ubuntu:19.04
RUN apt-get update
RUN apt-get install -y --no-install-recommends \
  gcc libc6-dev ca-certificates linux-headers-generic

RUN apt search linux-headers
RUN ls /usr/src

ENV PATH=$PATH:/rust/bin
