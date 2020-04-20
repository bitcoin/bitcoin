FROM veriblock/prerelease-btc
ADD . /app
WORKDIR /app
RUN export ALTINTEGRATION_VERSION=$(awk -F '=' '/\$\(package\)_version/{print $NF}' $PWD/depends/packages/altintegration.mk | head -n1); \
    (\
     cd /opt; \
     wget https://github.com/VeriBlock/alt-integration-cpp/archive/${ALTINTEGRATION_VERSION}.tar.gz; \
     tar -xf ${ALTINTEGRATION_VERSION}.tar.gz; \
     cd alt-integration-cpp-${ALTINTEGRATION_VERSION}; \
     mkdir build; \
     cd build; \
     cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_ROCKSDB=OFF -DTESTING=OFF; \
     make -j2 install \
    )
RUN ./autogen.sh
RUN CC=gcc-7 CXX=g++-7 ./configure --without-gui --disable-tests --disable-bench --disable-man --with-libs=no
RUN make -j6 install

WORKDIR /root

# some cleanup to decrease image size
RUN strip -s /usr/local/bin/* || true; \
    strip -s /usr/local/lib/* || true; \
    rm -rf /app
