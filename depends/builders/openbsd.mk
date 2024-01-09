build_openbsd_CC = clang
build_openbsd_CXX = clang++

build_openbsd_SHA256SUM = sha256
build_openbsd_DOWNLOAD = curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

build_openbsd_TAR = gtar
