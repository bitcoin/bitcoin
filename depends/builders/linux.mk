build_linux_SHA256SUM = sha256sum
build_linux_DOWNLOAD = wget --no-check-certificate --timeout=$(DOWNLOAD_CONNECT_TIMEOUT) --tries=$(DOWNLOAD_RETRIES) -nv -O
