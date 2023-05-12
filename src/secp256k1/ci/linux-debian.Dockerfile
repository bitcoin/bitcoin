FROM debian:stable

RUN dpkg --add-architecture i386 && \
    dpkg --add-architecture s390x && \
    dpkg --add-architecture armhf && \
    dpkg --add-architecture arm64 && \
    dpkg --add-architecture ppc64el

# dkpg-dev: to make pkg-config work in cross-builds
# llvm: for llvm-symbolizer, which is used by clang's UBSan for symbolized stack traces
RUN apt-get update && apt-get install --no-install-recommends -y \
        git ca-certificates \
        make automake libtool pkg-config dpkg-dev valgrind qemu-user \
        gcc clang llvm libc6-dbg \
        g++ \
        gcc-i686-linux-gnu libc6-dev-i386-cross libc6-dbg:i386 libubsan1:i386 libasan6:i386 \
        gcc-s390x-linux-gnu libc6-dev-s390x-cross libc6-dbg:s390x \
        gcc-arm-linux-gnueabihf libc6-dev-armhf-cross libc6-dbg:armhf \
        gcc-aarch64-linux-gnu libc6-dev-arm64-cross libc6-dbg:arm64 \
        gcc-powerpc64le-linux-gnu libc6-dev-ppc64el-cross libc6-dbg:ppc64el \
        gcc-mingw-w64-x86-64-win32 wine64 wine \
        gcc-mingw-w64-i686-win32 wine32 \
        sagemath

WORKDIR /root
# The "wine" package provides a convience wrapper that we need
RUN apt-get update && apt-get install --no-install-recommends -y \
        git ca-certificates wine64 wine python3-simplejson python3-six msitools winbind procps && \
    git clone https://github.com/mstorsjo/msvc-wine && \
    mkdir /opt/msvc && \
    python3 msvc-wine/vsdownload.py --accept-license --dest /opt/msvc Microsoft.VisualStudio.Workload.VCTools && \
    msvc-wine/install.sh /opt/msvc

# Initialize the wine environment. Wait until the wineserver process has
# exited before closing the session, to avoid corrupting the wine prefix.
RUN wine64 wineboot --init && \
    while (ps -A | grep wineserver) > /dev/null; do sleep 1; done
