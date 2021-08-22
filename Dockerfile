FROM alpine AS build

WORKDIR /bitcoin
COPY . . 

RUN apk --no-cache add libffi-dev openssl-dev bash coreutils
RUN apk --no-cache add --update alpine-sdk
RUN apk --no-cache add --update build-base
RUN apk --no-cache add autoconf automake libtool
RUN apk --no-cache add boost-dev libevent-dev 

RUN cd /bitcoin/depends; make NO_QT=1

RUN wget https://zlib.net/zlib-1.2.11.tar.gz
RUN mkdir -p /usr/src/zlib; tar zxvf zlib-1.2.11.tar.gz -C /usr/src
RUN cd /usr/src/zlib-1.2.11; ./configure; make; make install

ENV CONFIG_SITE=/bitcoin/depends/x86_64-pc-linux-musl/share/config.site
RUN cd /bitcoin; ./autogen.sh
RUN ./configure --disable-ccache \
    --disable-maintainer-mode \
    --disable-dependency-tracking \
    --enable-reduce-exports --disable-bench \
#    --disable-tests \
    --disable-gui-tests \
    --without-gui \
    --without-miniupnpc \
    CFLAGS="-O2 -g0 --static -static -fPIC" \
    CXXFLAGS="-O2 -g0 --static -static -fPIC" \
    LDFLAGS="-s -static-libgcc -static-libstdc++ -Wl,-O2"
RUN make
RUN make check
RUN make install

FROM scratch
COPY --from=build /usr/local /usr/local
COPY --from=build /bitcoin/share/examples/bitcoin.conf /.bitcoin/bitcoin.conf

VOLUME ["/.bitcoin"]

EXPOSE 8332 8333 18332 18333 18444

CMD ["/usr/local/bin/bitcoind", "-printtoconsole"]

