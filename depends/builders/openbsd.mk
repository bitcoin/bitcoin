build_openbsd_CC = clang
build_openbsd_CXX = clang++

build_openbsd_SHA256SUM = sha256 -r
build_openbsd_DOWNLOAD = curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

build_openbsd_TAR = gtar
# openBSD touch doesn't understand -h
build_openbsd_TOUCH = touch -m -t 200001011200

# openbsd host on openbsd builder: override openbsd host preferences.
openbsd_CC = clang
openbsd_CXX = clang++
