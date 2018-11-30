# ubuntu 16.04

this is working

## docker

```
SET UP THE REPOSITORY
sudo apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo apt-key fingerprint 0EBFCD88
should be response
pub   4096R/0EBFCD88 2017-02-22
      Key fingerprint = 9DC8 5822 9FC7 DD38 854A  E2D8 8D81 803C 0EBF CD88
uid                  Docker Release (CE deb) <docker@docker.com>
sub   4096R/F273FCD8 2017-02-22

sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"

INSTALL DOCKER CE
sudo apt-get update
sudo apt-get install docker-ce

verify docker installation
sudo docker run hello-world

// isntall ruby
sudo apt-get install ruby-full

// solve permission denied issue
sudo chmod a+rwx /var/run/docker.sock
sudo chmod a+rwx /var/run/docker.pid

git clone git://github.com/devrandom/gitian-builder.git
mkdir gitian-builder/inputs
cd gitian-builder/inputs
wget -O miniupnpc-1.6.tar.gz 'http://miniupnp.tuxfamily.org/files/download.php?file=miniupnpc-1.6.tar.gz'
wget 'http://fukuchi.org/works/qrencode/qrencode-3.2.0.tar.bz2'
# Inputs for Win32: (Linux has packages for these)
wget 'https://downloads.sourceforge.net/project/boost/boost/1.52.0/boost_1_52_0.tar.bz2'
wget 'http://www.openssl.org/source/openssl-1.0.1g.tar.gz'
wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
wget 'https://downloads.sourceforge.net/project/libpng/zlib/1.2.7/zlib-1.2.7.tar.gz'
wget 'https://downloads.sourceforge.net/project/libpng/libpng15/older-releases/1.5.12/libpng-1.5.12.tar.gz'
wget 'http://download.qt.io/archive/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz'

need modify make-base-vm add linux 32bit support

if [ -n "$DOCKER_IMAGE_HASH" ]; then
  base_image="$DISTRO@sha256:$DOCKER_IMAGE_HASH"
  OUT=base-$DOCKER_IMAGE_HASH-$ARCH
else
  if [ $ARCH = "i386" ]; then
    base_image="i386/$DISTRO:$SUITE"
  else
    base_image="$DISTRO:$SUITE"
  fi
fi

bin/make-base-vm --docker --arch amd64 --suite bionic
bin/make-base-vm --docker --arch i386 --suite bionic

// linux temperorily use this
bin/make-base-vm --docker --arch amd64 --suite trusty
bin/make-base-vm --docker --arch i386 --suite trusty

export USE_DOCKER=1

// build linux approximately 30min mbp 2015
./bin/gbuild -j4 --commit primecoin=a73f8922475f6c7dc92f1143edc166b28cd2b786 ../primecoin/contrib/gitian-descriptors/gitian.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/gitian.yml

// build from github source.
./bin/gbuild -j4 --commit primecoin=HEAD ../primecoin/contrib/gitian-descriptors/gitian_remote.yml

// build windows
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/boost-win64.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/deps-win64.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/qt-win64.yml

./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/boost-win32.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/deps-win32.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/qt-win32.yml

// if run in mac need modify gbuild line 147 date parameters is different with linux
author_date = `cd inputs/#{dir} && v2=$(git log --format=%at -1) && date -r $v2 +'%Y-%m-%d %H:%M:%S'`.strip

If you use local code build need modify code path in *.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/gitian-win64.yml
./bin/gbuild -j4 --commit primecoin=v0.1.5xpm ../primecoin/contrib/gitian-descriptors/gitian-win32.yml
```
