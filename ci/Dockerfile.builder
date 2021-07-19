FROM ubuntu:bionic

# Build and base stuff
# (zlib1g-dev and libssl-dev are needed for the Qt host binary builds, but should not be used by target binaries)
# We split this up into multiple RUN lines as we might need to retry multiple times on Travis. This way we allow better
# cache usage.
ENV APT_ARGS="-y --no-install-recommends --no-upgrade"
RUN apt-get update && apt-get install $APT_ARGS git wget unzip && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS g++ && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS autotools-dev libtool m4 automake autoconf pkg-config && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS zlib1g-dev libssl1.0-dev curl ccache bsdmainutils cmake && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS python3 python3-dev && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS python3-pip python3-setuptools && rm -rf /var/lib/apt/lists/*

# Python stuff
RUN pip3 install pyzmq # really needed?
RUN pip3 install jinja2
RUN pip3 install flake8==3.5.0
RUN pip3 install codespell==1.13.0

# dash_hash
RUN git clone https://github.com/dashpay/dash_hash
RUN cd dash_hash && python3 setup.py install

ARG USER_ID=1000
ARG GROUP_ID=1000

# add user with specified (or default) user/group ids
ENV USER_ID ${USER_ID}
ENV GROUP_ID ${GROUP_ID}
RUN groupadd -g ${GROUP_ID} dash
RUN useradd -u ${USER_ID} -g dash -s /bin/bash -m -d /dash dash

# Packages needed for all target builds
RUN dpkg --add-architecture i386
RUN apt-get update && apt-get install $APT_ARGS g++-7-multilib && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS g++-arm-linux-gnueabihf && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS g++-mingw-w64-i686 && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS g++-mingw-w64-x86-64 && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS wine-stable wine32 wine64 bc nsis && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS python3-zmq && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS shellcheck && rm -rf /var/lib/apt/lists/*
RUN apt-get update && apt-get install $APT_ARGS imagemagick libcap-dev librsvg2-bin libz-dev libbz2-dev libtiff-tools && rm -rf /var/lib/apt/lists/*

# This is a hack. It is needed because gcc-multilib and g++-multilib are conflicting with g++-arm-linux-gnueabihf. This is
# due to gcc-multilib installing the following symbolic link, which is needed for -m32 support. However, this causes
# arm builds to also have the asm folder implicitly in the include search path. This is kind of ok, because the asm folder
# for arm has precedence.
RUN ln -s x86_64-linux-gnu/asm /usr/include/asm

# Make sure std::thread and friends is available
RUN \
  update-alternatives --set i686-w64-mingw32-gcc /usr/bin/i686-w64-mingw32-gcc-posix; \
  update-alternatives --set i686-w64-mingw32-g++  /usr/bin/i686-w64-mingw32-g++-posix; \
  update-alternatives --set x86_64-w64-mingw32-gcc  /usr/bin/x86_64-w64-mingw32-gcc-posix; \
  update-alternatives --set x86_64-w64-mingw32-g++  /usr/bin/x86_64-w64-mingw32-g++-posix; \
  exit 0

RUN mkdir /dash-src && \
  mkdir -p /cache/ccache && \
  mkdir /cache/depends && \
  mkdir /cache/sdk-sources && \
  chown $USER_ID:$GROUP_ID /dash-src && \
  chown $USER_ID:$GROUP_ID /cache && \
  chown $USER_ID:$GROUP_ID /cache -R
WORKDIR /dash-src

USER dash
