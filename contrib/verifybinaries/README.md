### Verify Binaries

#### Usage:

This script attempts to download the signature file `SHA256SUMS.asc` from https://syscoin.org.

It first checks if the signature passes, and then downloads the files specified in the file, and checks if the hashes of these files match those that are specified in the signature file.

The script returns 0 if everything passes the checks. It returns 1 if either the signature check or the hash check doesn't pass. If an error occurs the return value is 2.


```sh
./verify.py syscoin-core-0.11.2
./verify.py syscoin-core-0.12.0
./verify.py syscoin-core-0.13.0-rc3
```

If you only want to download the binaries of certain platform, add the corresponding suffix, e.g.:

```sh
./verify.py syscoin-core-0.11.2-osx
./verify.py 0.12.0-linux
./verify.py syscoin-core-0.13.0-rc3-win64
```

If you do not want to keep the downloaded binaries, specify anything as the second parameter.

```sh
./verify.py syscoin-core-0.13.0 delete
```
