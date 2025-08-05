########################################
# ----------- CONFIGURATION -----------
########################################

ARG TARGETARCH
ARG VERSION=29.0

#############################################
# ----------- SELECT SYSTEM ARCH -----------
#############################################

FROM debian:bookworm-slim AS debian_settings
ENV DEBIAN_FRONTEND=noninteractive

FROM debian_settings AS debian_amd64
ENV ARCH=x86_64
FROM debian_settings AS debian_arm64
ENV ARCH=aarch64
FROM debian_settings AS debian_riscv64
ENV ARCH=riscv64

FROM debian_${TARGETARCH} AS base-system

########################################
# ---------- BUILD BASE --------------
########################################

FROM base-system AS build-base

RUN apt update && \
    apt install -y --no-install-recommends build-essential cmake git pkgconf python3 \
        libevent-dev libboost-dev libzmq3-dev \
        systemtap-sdt-dev ca-certificates curl \
        libcapnp-dev capnproto && \
    apt clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

###################################################
# ----------- BUILD FROM OFFICIAL SITE -----------
###################################################

FROM base-system AS build-official-site

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates gnupg wget tar && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

ARG VERSION
ENV VERSION="${VERSION}"

ENV BITCOIN_CORE_SIGNATURE=71A3B16735405025D447E8F274810B012346C9A6

RUN gpg --keyserver hkp://keyserver.ubuntu.com --recv-keys ${BITCOIN_CORE_SIGNATURE}
RUN wget https://bitcoincore.org/bin/bitcoin-core-${VERSION}/SHA256SUMS.asc 
RUN wget https://bitcoincore.org/bin/bitcoin-core-${VERSION}/SHA256SUMS
RUN wget https://bitcoincore.org/bin/bitcoin-core-${VERSION}/bitcoin-${VERSION}-${ARCH}-linux-gnu.tar.gz
RUN gpg --status-fd 1 --verify SHA256SUMS.asc SHA256SUMS 2>/dev/null | grep "^\[GNUPG:\] VALIDSIG.*${BITCOIN_CORE_SIGNATURE}\$" 
RUN sha256sum --ignore-missing --check SHA256SUMS 
RUN tar -xzvf bitcoin-${VERSION}-${ARCH}-linux-gnu.tar.gz -C /opt 
RUN ln -sv bitcoin-${VERSION} /opt/bitcoin
RUN rm -v /opt/bitcoin/bin/test_bitcoin /opt/bitcoin/bin/bitcoin-qt

#####################################
# -------- BUILD FROM GITHUB -------
#####################################

FROM build-base AS build-github

WORKDIR /src

ARG VERSION
ENV VERSION="${VERSION}"

RUN git clone --branch v${VERSION} --depth 1 https://github.com/bitcoin/bitcoin.git .

RUN cmake -B build -DENABLE_WALLET=OFF -DBUILD_GUI=OFF -DWITH_QRENCODE=OFF -DWITH_ZMQ=ON -DENABLE_IPC=ON . && \
    cmake --build build --parallel $(nproc)

RUN build/bin/test_bitcoin --show_progress 
RUN rm -v /opt/bitcoin/bin/test_bitcoin /opt/bitcoin/bin/bitcoin-qt

####################################
# -------- BUILD FROM LOCAL -------
####################################

FROM build-base AS build-local

WORKDIR /src

COPY . .

RUN cmake -B build -DENABLE_WALLET=OFF -DBUILD_GUI=OFF -DWITH_QRENCODE=OFF -DWITH_ZMQ=ON -DENABLE_IPC=ON . && \
    cmake --build build --parallel $(nproc)

RUN build/bin/test_bitcoin --show_progress 
RUN rm -v /opt/bitcoin/bin/test_bitcoin /opt/bitcoin/bin/bitcoin-qt


#############################################
# ---------- FINAL BASE IMAGE --------------
#############################################

FROM base-system AS final-base

RUN apt-get update && apt-get install -y --no-install-recommends \
    gosu libatomic1 libevent-2.1-7 libzmq5 ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV HOME=/bitcoin
VOLUME ["/bitcoin/.bitcoin"]
WORKDIR /bitcoin

ARG GROUP_ID=1000
ARG USER_ID=1000

RUN groupadd -g ${GROUP_ID} bitcoin && useradd -u ${USER_ID} -g bitcoin -d /bitcoin bitcoin

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 8332 8333
ENTRYPOINT ["/entrypoint.sh"]

##############################################
# ---------- OFFICIAL SITE IMAGE ------------
##############################################

FROM final-base AS official-site
COPY --from=build-official-site /opt/bitcoin/bin/bitcoind /usr/local/bin/bitcoind
COPY --from=build-official-site /opt/bitcoin/bin/bitcoin-cli /usr/local/bin/bitcoin-cli

#######################################
# ---------- GITHUB IMAGE ------------
########################################

FROM final-base AS github
COPY --from=build-github /opt/bitcoin/bin/bitcoind /usr/local/bin/bitcoind
COPY --from=build-github /opt/bitcoin/bin/bitcoin-cli /usr/local/bin/bitcoin-cli

######################################
# ---------- LOCAL IMAGE ------------
######################################

FROM final-base AS local
COPY --from=build-local /opt/bitcoin/bin/bitcoind /usr/local/bin/bitcoind
COPY --from=build-local /opt/bitcoin/bin/bitcoin-cli /usr/local/bin/bitcoin-cli
