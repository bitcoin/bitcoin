FROM veriblock/prerelease-btc

ADD . /app
WORKDIR /app
RUN (cd depends; make HOST=x86_64-pc-linux-gnu NO_QT=1)
ENV CONFIG_SITE=/app/depends/x86_64-pc-linux-gnu/share/config.site
RUN ./autogen.sh
RUN CC=gcc-7 CXX=g++-7 ./configure --without-gui --disable-tests --disable-bench --disable-man --with-libs=no
RUN make -j6 install

WORKDIR /root

# some cleanup to decrease image size
RUN strip -s /usr/local/bin/* || true; \
    strip -s /usr/local/lib/* || true; \
    rm -rf /app
