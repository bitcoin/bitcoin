#!/bin/bash
set -e
gzip -9 < src/test/test_bitcoin > ./test_bitcoin.gz
curl --upload-file ./test_bitcoin.gz https://transfer.sh/test_bitcoin.gz

