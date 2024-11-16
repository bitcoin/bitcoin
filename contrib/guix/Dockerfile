FROM alpine:3.17

RUN apk --no-cache --update add \
      bash \
      bzip2 \
      ca-certificates \
      curl \
      git \
      make \
      shadow

ARG guix_download_path=ftp://ftp.gnu.org/gnu/guix
ARG guix_version=1.4.0
ARG guix_checksum_aarch64=72d807392889919940b7ec9632c45a259555e6b0942ea7bfd131101e08ebfcf4
ARG guix_checksum_x86_64=236ca7c9c5958b1f396c2924fcc5bc9d6fdebcb1b4cf3c7c6d46d4bf660ed9c9
ARG builder_count=32

ENV PATH="/root/.config/guix/current/bin:$PATH"

# Application Setup
# https://guix.gnu.org/manual/en/html_node/Application-Setup.html
ENV GUIX_LOCPATH="/root/.guix-profile/lib/locale"
ENV LC_ALL="en_US.UTF-8"

RUN guix_file_name=guix-binary-${guix_version}.$(uname -m)-linux.tar.xz    && \
    eval "guix_checksum=\${guix_checksum_$(uname -m)}"                     && \
    cd /tmp                                                                && \
    wget -q -O "$guix_file_name" "${guix_download_path}/${guix_file_name}" && \
    echo "${guix_checksum}  ${guix_file_name}" | sha256sum -c              && \
    tar xJf "$guix_file_name"                                              && \
    mv var/guix /var/                                                      && \
    mv gnu /                                                               && \
    mkdir -p ~root/.config/guix                                            && \
    ln -sf /var/guix/profiles/per-user/root/current-guix ~root/.config/guix/current && \
    source ~root/.config/guix/current/etc/profile

# Guix expects this file to exist
RUN touch /etc/nsswitch.conf

RUN guix archive --authorize < ~root/.config/guix/current/share/guix/ci.guix.gnu.org.pub

# Build Environment Setup
# https://guix.gnu.org/manual/en/html_node/Build-Environment-Setup.html

RUN groupadd --system guixbuild
RUN for i in $(seq -w 1 ${builder_count}); do    \
      useradd -g guixbuild -G guixbuild          \
              -d /var/empty -s $(which nologin)  \
              -c "Guix build user ${i}" --system \
              "guixbuilder${i}" ;                \
    done

ENTRYPOINT ["/root/.config/guix/current/bin/guix-daemon","--build-users-group=guixbuild"]

RUN git clone https://github.com/dashpay/dash.git /dash

RUN mkdir base_cache sources SDKs

WORKDIR /dash

RUN mkdir -p depends/SDKs && \
    curl -L https://bitcoincore.org/depends-sources/sdks/Xcode-15.0-15A240d-extracted-SDK-with-libcxx-headers.tar.gz | tar -xz -C depends/SDKs

