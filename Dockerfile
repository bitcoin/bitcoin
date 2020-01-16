FROM veriblock/prerelease-btc

ADD . /app
WORKDIR /app
RUN ./autogen.sh
RUN CC=gcc-7 CXX=g++-7 ./configure --without-gui --disable-tests --disable-bench --disable-man --with-libs=no
RUN make -j6 install

WORKDIR /root

# some cleanup to decrease image size
RUN strip -s /usr/local/bin/* || true; \
    strip -s /usr/local/lib/* || true; \
    rm -rf /app
