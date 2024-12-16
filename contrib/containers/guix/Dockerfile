# Note: Using 'docker compose up' will leave you hanging, you need
#       to use 'docker compose run guix_ubuntu' to drop into an
#       interactive shell

FROM ubuntu:jammy

SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends --no-upgrade \
    build-essential \
    bzip2 \
    ca-certificates \
    curl \
    git \
    locales \
    netbase \
    sudo \
    wget \
    xz-utils && \
    rm -rf /var/lib/apt/lists/*; \
    targetLocale="en_US.UTF-8"; \
    locale-gen ${targetLocale} && \
    update-locale LC_ALL=${targetLocale} && \
    update-locale LANG=${targetLocale};

ARG guix_download_path=ftp://ftp.gnu.org/gnu/guix
ARG guix_version=1.4.0
ARG guix_checksum_aarch64=72d807392889919940b7ec9632c45a259555e6b0942ea7bfd131101e08ebfcf4
ARG guix_checksum_x86_64=236ca7c9c5958b1f396c2924fcc5bc9d6fdebcb1b4cf3c7c6d46d4bf660ed9c9
ARG builder_count=32

ENV PATH="/usr/local/bin:/usr/local/guix/current/bin:$PATH"

# Application Setup
# https://guix.gnu.org/manual/en/html_node/Application-Setup.html
ENV GUIX_LOCPATH="/usr/local/guix/profile"

RUN guix_file_name=guix-binary-${guix_version}.$(uname -m)-linux.tar.xz          && \
    eval "guix_checksum=\${guix_checksum_$(uname -m)}"                           && \
    cd /tmp                                                                      && \
    wget -q -O "$guix_file_name" "${guix_download_path}/${guix_file_name}"       && \
    echo "${guix_checksum}  ${guix_file_name}" | sha256sum -c                    && \
    tar xJf "$guix_file_name"                                                    && \
    mv var/guix /var/                                                            && \
    mv gnu /                                                                     && \
    mkdir -p /usr/local/guix                                                     && \
    ln -sf /var/guix/profiles/per-user/root/current-guix /usr/local/guix/current && \
    ln -sf /var/guix/profiles/per-user/root/guix-profile /usr/local/guix/profile && \
    chmod 1777 /tmp /var/tmp                                                     && \
    source /usr/local/guix/current/etc/profile

RUN touch /etc/nsswitch.conf

RUN guix archive --authorize < /usr/local/guix/current/share/guix/ci.guix.gnu.org.pub && \
    guix archive --authorize < /usr/local/guix/current/share/guix/bordeaux.guix.gnu.org.pub

# Build Environment Setup
# https://guix.gnu.org/manual/en/html_node/Build-Environment-Setup.html
RUN groupadd --system guixbuild && \
    for i in $(seq -w 1 ${builder_count}); do    \
      useradd -g guixbuild -G guixbuild          \
              -d /var/empty -s $(which nologin)  \
              -c "Guix build user ${i}" --system \
              "guixbuilder${i}" ;                \
    done

# Create unprivileged user
ARG USER_ID=1000 \
    GROUP_ID=1000 \
    USERNAME=ubuntu
RUN groupadd -g ${GROUP_ID} ${USERNAME} && \
    useradd -u ${USER_ID} -g ${USERNAME} -s /bin/bash -m -d /home/${USERNAME} ${USERNAME}

# Grant it passwordless admin permissions
RUN usermod -aG sudo ${USERNAME} && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Copy required files to container
COPY --from=docker_root ./motd.txt             /etc/motd
COPY --from=docker_root ./scripts/entrypoint   /usr/local/bin/entrypoint
COPY --from=docker_root ./scripts/guix-check   /usr/local/bin/guix-check
COPY --from=docker_root ./scripts/guix-start   /usr/local/bin/guix-start
COPY --from=docker_root ./scripts/setup-sdk    /usr/local/bin/setup-sdk

# Create directories for mounting to save/restore cache and grant necessary permissions
RUN mkdir -p \
        /home/${USERNAME}/.cache \
        /src/dash/depends/{built,sources,work}; \
    chown -R ${USER_ID}:${GROUP_ID} \
        /home/${USERNAME}/.cache \
        /src;

WORKDIR "/src/dash"

# Switch to unprivileged context
USER ${USERNAME}

# Set entrypoint to copied file
ENTRYPOINT ["/usr/local/bin/entrypoint"]
