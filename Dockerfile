FROM gcr.io/sebible/btc-build-essential
RUN mkdir -p /data
ADD . /data/
WORKDIR /data
RUN cd /data && ./autogen.sh && ./configure --disable-wallet && make && make install
