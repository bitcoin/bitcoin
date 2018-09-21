FROM ubuntu:bionic

# Build and base stuff
# (zlib1g-dev and libssl-dev are needed for the Qt host binary builds, but should not be used by target binaries)
RUN apt-get update && apt-get install -y \
  git \
  g++ \
  autotools-dev libtool m4 automake autoconf pkg-config \
  zlib1g-dev \
  libssl1.0-dev \
  curl \
  ccache \
  bsdmainutils \
  python3-pip python3-dev \
  cmake \
  && rm -rf /var/lib/apt/lists

# Python stuff
RUN pip3 install pyzmq # really needed?

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

# Extra packages
ARG BUILD_TARGET=linux64
ADD matrix.sh /tmp/matrix.sh
RUN . /tmp/matrix.sh && \
  if [ -n "$DPKG_ADD_ARCH" ]; then dpkg --add-architecture "$DPKG_ADD_ARCH" ; fi && \
  if [ -n "$PACKAGES" ]; then apt-get update && apt-get install -y --no-install-recommends --no-upgrade $PACKAGES && rm -rf /var/lib/apt/lists; fi

# Make sure std::thread and friends is available
# Will fail on non-win builds, but we ignore this
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
