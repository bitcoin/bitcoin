build_freebsd_CC=clang
build_freebsd_CXX=clang++

build_freebsd_SHA256SUM = sha256sum
build_freebsd_DOWNLOAD = curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o
