# Build stage for Bitcoin Core

FROM --platform=$BUILDPLATFORM alpine:3.21 AS base-system

FROM base-system AS build

RUN apk --no-cache add \
    boost-dev \
    build-base \
    ccache \
    chrpath \
    clang18 \
    cmake \
    file \
    gnupg \
    git \
    libevent-dev \
    libtool \
    linux-headers \
    sqlite-dev \
    zeromq-dev

ARG BITCOIN_VERSION=29.0
ENV BITCOIN_VERSION="${BITCOIN_VERSION}"
ENV BITCOIN_PREFIX=/opt/bitcoin-${BITCOIN_VERSION}
ENV BITCOIN_SOURCE_DIR=/bitcoin/src
ENV SIGS_REPO_URL="https://github.com/bitcoin-core/guix.sigs.git"
ENV SIGS_CLONE_DIR="guix.sigs"
ENV VERIFY_SCRIPT_URL="https://raw.githubusercontent.com/bitcoin/bitcoin/v${BITCOIN_VERSION}/contrib/verify-binaries/verify.py"

WORKDIR /bitcoin
COPY ./contrib/verify-binaries/verify.py .

RUN set -ex \
  && if echo $BITCOIN_VERSION | grep -q "rc" ; then \
       VERIFY_VERSION=$(echo $BITCOIN_VERSION | sed 's/\(.*\)rc\([0-9]*\)/\1-rc\2/'); \
       ADDRESS="https://bitcoincore.org/bin/bitcoin-core-${BITCOIN_VERSION%%rc*}/test.rc${BITCOIN_VERSION##*rc}"; \
     else \
       VERIFY_VERSION=$BITCOIN_VERSION; \
       ADDRESS="https://bitcoincore.org/bin/bitcoin-core-${VERIFY_VERSION}"; \
     fi \
  && echo "$VERIFY_VERSION" \
  && wget ${ADDRESS}/bitcoin-${BITCOIN_VERSION}.tar.gz \
  && wget ${ADDRESS}/SHA256SUMS \
  && wget ${ADDRESS}/SHA256SUMS.asc \
  && git clone ${SIGS_REPO_URL} ${SIGS_CLONE_DIR} \
  && gpg --import "${SIGS_CLONE_DIR}"/builder-keys/* \
  && ./verify.py bin SHA256SUMS \
    "bitcoin-${BITCOIN_VERSION}.tar.gz" \
  && mkdir -p ${BITCOIN_SOURCE_DIR} \
  && tar -xzf "bitcoin-${BITCOIN_VERSION}.tar.gz" -C ${BITCOIN_SOURCE_DIR} \
  && rm -rf ${SIGS_CLONE_DIR}

WORKDIR "${BITCOIN_SOURCE_DIR}/bitcoin-${BITCOIN_VERSION}"

RUN cmake -B build \
    -DBUILD_TESTS=OFF \
    -DBUILD_TX=ON \
    -DBUILD_UTIL=OFF \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_C_COMPILER=clang-18 \
    -DCMAKE_INSTALL_PREFIX:PATH="${BITCOIN_PREFIX}" \
    -DWITH_CCACHE=ON && \
    cmake --build build -j$(nproc) && \
    strip build/bin/bitcoin-cli build/bin/bitcoin-tx build/bin/bitcoind && \
    cmake --install build

# Build stage for compiled artifacts
FROM base-system

ARG UID=100
ARG GID=101

LABEL maintainer.0="Will Clark (@willcl-ark)"

RUN addgroup bitcoin --gid ${GID} --system
RUN adduser --uid ${UID} --system bitcoin --ingroup bitcoin
RUN apk --no-cache add \
  libevent \
  libzmq \
  shadow \
  sqlite-libs \
  su-exec

ENV BITCOIN_DATA=/home/bitcoin/.bitcoin
ENV BITCOIN_VERSION=29.0
ENV BITCOIN_PREFIX=/opt/bitcoin-${BITCOIN_VERSION}
ENV PATH=/opt/bin:$PATH

COPY --from=build ${BITCOIN_PREFIX} /opt

COPY ./contrib/docker/bitcoin/entrypoint.sh /entrypoint.sh

RUN chmod +x /entrypoint.sh

RUN if echo "$BITCOIN_VERSION" | grep -q "rc"; then \
    PADDED_VERSION=$(echo $BITCOIN_VERSION | sed 's/\([0-9]\+\)\.\([0-9]\+\)rc/\1.\2.0rc/'); \
  else \
    PADDED_VERSION=$BITCOIN_VERSION; \
  fi

VOLUME ["/home/bitcoin/.bitcoin"]

EXPOSE 8332 8333 18332 18333 18444

ENTRYPOINT ["/entrypoint.sh"]

RUN bitcoind -version | grep "Bitcoin Core daemon version v${PADDED_VERSION}"

CMD ["bitcoind"]