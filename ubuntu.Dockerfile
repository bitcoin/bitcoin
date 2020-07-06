FROM veriblock/prerelease-btc

ADD . /app
WORKDIR /app
ARG BUILD_TYPE=Release

RUN pip3 install cmake

ENV BUILD=${BUILD_TYPE}
RUN export VERIBLOCK_POP_CPP_VERSION=$(awk -F '=' '/\$\(package\)_version/{print $NF}' $PWD/depends/packages/veriblock-pop-cpp.mk | head -n1); \
    (\
     cd /opt; \
     wget https://github.com/VeriBlock/alt-integration-cpp/archive/${VERIBLOCK_POP_CPP_VERSION}.tar.gz; \
     tar -xf ${VERIBLOCK_POP_CPP_VERSION}.tar.gz; \
     cd alt-integration-cpp-${VERIBLOCK_POP_CPP_VERSION}; \
     mkdir build; \
     cd build; \
     cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DTESTING=OFF; \
     make -j2 install \
    )

RUN ./autogen.sh
RUN if [ ${BUILD_TYPE} = "Release" ]; then \
      CC=gcc-7 CXX=g++-7 ./configure \
        --without-gui \
        --disable-tests \
        --disable-bench \
        --disable-ccache \
        --disable-man \
        --with-libs=no; \
    else \
      CC=gcc-7 CXX=g++-7 ./configure \
        --enable-debug \
        --without-gui \
        --disable-tests \
        --disable-bench \
        --disable-ccache \
        --disable-man \
        --with-libs=no; \
    fi
RUN make -j4 install
# remove source files to decrease image size
RUN rm -rf /app
ENV DATA_DIR=/home/vbitcoin/.vbitcoin
RUN groupadd -r --gid 1001 vbitcoin
RUN useradd --no-log-init -r --uid 1001 --gid 1001 --create-home --shell /bin/bash vbitcoin
RUN mkdir -p ${DATA_DIR}
RUN chown -R 1001:1001 ${DATA_DIR}
USER vbitcoin

WORKDIR $DATA_DIR
