#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

SSL=${SSL:-no}
INSTALL_DIR=$(pwd)/install-dir

install_openssl_fips() {
   curl --retry 5 -o fips.tar.gz https://www.openssl.org/source/openssl-fips-2.0.14.tar.gz
   tar zxvf fips.tar.gz
   cd openssl-fips-2.0.14
   ./config --prefix=$INSTALL_DIR -fPIC
   cpus=$(grep -c '^processor' /proc/cpuinfo)
   make -j${cpus} || true
   make install || true
   cd ..
   SSL_EXTRA_FLAGS="--openssldir=$INSTALL_DIR --with-fipsdir=$INSTALL_DIR fips"
   SSL=${SSL%-fips}
}
install_openssl () {
   SSL_VERSION=${SSL##openssl-}
   tmp=$(echo $SSL_VERSION | tr . _)
   curl -L --retry 5 -o ssl.tar.gz https://github.com/openssl/openssl/archive/OpenSSL_${tmp}.tar.gz
   tar zxvf ssl.tar.gz
   cd openssl-OpenSSL_$tmp
   ./config --prefix=$INSTALL_DIR $SSL_EXTRA_FLAGS shared -fPIC
   cpus=$(grep -c '^processor' /proc/cpuinfo)
   make -j32 || true
   make -j${cpus} || true
   make install || true
   cd ..

   export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig
   export EXTRA_LIB_PATH="$INSTALL_DIR/lib"
   SSL="openssl";
}

install_libressl () {
   curl --retry 5 -o ssl.tar.gz https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/$SSL.tar.gz
   tar zxvf ssl.tar.gz
   cd $SSL
   ./configure --prefix=$INSTALL_DIR
   cpus=$(grep -c '^processor' /proc/cpuinfo)
   make -j${cpus}
   make install
   cd ..

   export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig
   export EXTRA_LIB_PATH="$INSTALL_DIR/lib"
   SSL="libressl";
}
# Get the kernel name, lowercased
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "OS: $OS"

# Automatically retrieve the machine architecture, lowercase, unless provided
# as an environment variable (e.g. to force 32bit)
[ -z "$MARCH" ] && MARCH=$(uname -m | tr '[:upper:]' '[:lower:]')

case "$SSL" in
   openssl-*-fips)
      install_openssl_fips;
      install_openssl;
   ;;

   openssl-*)
      install_openssl;
  ;;

  libressl-*)
     install_libressl;
     ;;
esac


