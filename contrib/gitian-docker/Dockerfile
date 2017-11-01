FROM ubuntu:14.04
RUN apt-get update 
RUN apt-get install -y cpu-checker
RUN apt-get install -y git apache2 apt-cacher-ng python-vm-builder ruby qemu-utils qemu-kvm
RUN apt-get install -y zip

RUN mkdir /root/.ssh/
RUN touch /root/.ssh/known_hosts
RUN ssh-keyscan github.com >> /root/.ssh/known_hosts
RUN git clone git://github.com/devrandom/gitian-builder.git

RUN mkdir -p gitian-builder/inputs
RUN cd gitian-builder/inputs && wget 'http://miniupnp.free.fr/files/download.php?file=miniupnpc-2.0.20170509.tar.gz' -O miniupnpc-2.0.20170509.tar.gz
RUN cd gitian-builder/inputs && wget 'http://www.openssl.org/source/openssl-1.0.1i.tar.gz'
RUN cd gitian-builder/inputs && wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
RUN cd gitian-builder/inputs && wget 'http://zlib.net/zlib-1.2.8.tar.gz'
RUN cd gitian-builder/inputs && wget 'ftp://ftp.simplesystems.org/pub/png/src/history/libpng16/libpng-1.6.8.tar.gz'
RUN cd gitian-builder/inputs && wget 'http://fukuchi.org/works/qrencode/qrencode-3.4.3.tar.bz2'
RUN cd gitian-builder/inputs && wget 'http://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.bz2'
RUN cd gitian-builder/inputs && wget 'https://svn.boost.org/trac/boost/raw-attachment/ticket/7262/boost-mingw.patch' -O boost-mingw-gas-cross-compile-2013-03-03.patch
RUN cd gitian-builder/inputs && wget 'https://download.qt-project.org/archive/qt/4.8/4.8.5/qt-everywhere-opensource-src-4.8.5.tar.gz'
RUN cd gitian-builder/inputs && wget 'http://curl.haxx.se/download/curl-7.41.0.tar.gz'

# extra macosx inputs 
RUN cd gitian-builder/inputs && wget 'https://github.com/phracker/MacOSX-SDKs/releases/download/10.13/MacOSX10.10.sdk.tar.xz'
RUN cd gitian-builder/inputs && wget 'http://master.qt.io/archive/qt/5.5/5.5.0/single/qt-everywhere-opensource-src-5.5.0.tar.gz'

ADD setup.sh /setup.sh
RUN chmod +x /setup.sh
