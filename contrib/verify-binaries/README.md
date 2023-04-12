### Verify Binaries

#### Preparation

As of Bitcoin Core v22.0, releases are signed by a number of public keys on the basis
of the [guix.sigs repository](https://github.com/bitcoin-core/guix.sigs/). When
verifying binary downloads, you (the end user) decide which of these public keys you
trust and then use that trust model to evaluate the signature on a file that contains
hashes of the release binaries. The downloaded binaries are then hashed and compared to
the signed checksum file.

First, you have to figure out which public keys to recognize. Browse the [list of frequent
builder-keys](https://github.com/bitcoin-core/guix.sigs/tree/main/builder-keys) and
decide which of these keys you would like to trust. For each key you want to trust, you
must obtain that key for your local GPG installation.

You can obtain these keys by
  - through a browser using a key server (e.g. keyserver.ubuntu.com),
  - manually using the `gpg --keyserver <url> --recv-keys <key>` command, or
  - you can run the packaged `verify.py ... --import-keys` script to
    have it automatically retrieve unrecognized keys.

#### Usage

This script attempts to download the checksum file (`SHA256SUMS`) and corresponding
signature file `SHA256SUMS.asc` from https://bitcoincore.org and https://bitcoin.org.

It first checks if the checksum file is valid based upon a plurality of signatures, and
then downloads the release files specified in the checksum file, and checks if the
hashes of the release files are as expected.

If we encounter pubkeys in the signature file that we do not recognize, the script
can prompt the user as to whether they'd like to download the pubkeys. To enable
this behavior, use the `--import-keys` flag.

The script returns 0 if everything passes the checks. It returns 1 if either the
signature check or the hash check doesn't pass. An exit code of >2 indicates an error.

See the `Config` object for various options.

#### Examples

Validate releases with default settings:
```sh
./contrib/verify-binaries/verify.py pub 22.0
./contrib/verify-binaries/verify.py pub 22.0-rc3
```

Get JSON output and don't prompt for user input (no auto key import):

```sh
./contrib/verify-binaries/verify.py --json pub 22.0-x86
```

Rely only on local GPG state and manually specified keys, while requiring a
threshold of at least 10 trusted signatures:
```sh
./contrib/verify-binaries/verify.py \
    --trusted-keys 74E2DEF5D77260B98BC19438099BAD163C70FBFA,9D3CC86A72F8494342EA5FD10A41BDC3F4FAFF1C \
    --min-good-sigs 10 pub 22.0-x86
```

If you only want to download the binaries for a certain platform, add the corresponding suffix, e.g.:

```sh
./contrib/verify-binaries/verify.py pub 24.0.1-darwin
./contrib/verify-binaries/verify.py pub 23.1-rc1-win64
```

If you do not want to keep the downloaded binaries, specify the cleanup option.

```sh
./contrib/verify-binaries/verify.py pub --cleanup 22.0
```

Use the bin subcommand to verify all files listed in a local checksum file

```sh
./contrib/verify-binaries/verify.py bin SHA256SUMS
```

Verify only a subset of the files listed in a local checksum file

```sh
./contrib/verify-binaries/verify.py bin ~/Downloads/SHA256SUMS \
    ~/Downloads/bitcoin-24.0.1-x86_64-linux-gnu.tar.gz \
    ~/Downloads/bitcoin-24.0.1-arm-linux-gnueabihf.tar.gz
```
