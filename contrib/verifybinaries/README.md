### Verify Binaries
This script attempts to download the signature file `SHA256SUMS.asc` from https://bitcoin.org.

It first checks if the signature passes, and then downloads the files specified in the file, and checks if the hashes of these files match those that are specified in the signature file.

The script returns 0 if everything passes the checks. It returns 1 if either the signature check or the hash check doesn't pass. If an error occurs the return value is 2.

Usage:

```sh
./verify.sh bitcoin-core-0.11.2
./verify.sh bitcoin-core-0.12.0
```
