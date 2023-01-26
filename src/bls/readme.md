[![Build Status](https://github.com/herumi/bls/actions/workflows/main.yml/badge.svg)](https://github.com/herumi/bls/actions/workflows/main.yml)

# BLS threshold signature

This library is an implementation of BLS threshold signature,
which supports the new BLS Signatures specified at [Ethereum 2.0 Phase 0](https://github.com/ethereum/eth2.0-specs/blob/dev/specs/phase0/beacon-chain.md#bls-signatures).

## News
- 2021/Sep/16 update mcl and improve performance of isValidOrder, which is called from setStr/deserialize.
- 2021/Apr/28 add blsSetGeneratorOfPublicKey to change the generator.
- 2021/Jan/28 check zero public key on BLS_ETH mode
- 2020/Oct/07 add `blsMultiVerify` to process many verification all togather with multi thread.

## Support architectures

- Windows Visual Studio / MSYS2(MinGW-w64)
- Linux
- macOS
- Android
- iOS
- WebAssembly

## Choice of groups

This library supports type-3 pairings such as BN curves and BLS curves.
G1, G2, and GT are a cyclic group of prime order r.
```
e : G1 x G2 -> GT ; pairing
```

There are two ways for BLS signature.

type|SecretKey|PublicKey|Signature|
-|-|-|-|
default|Fr|G2|G1|
ETH2.0 spec (BLS_ETH=1)|Fr|G1|G2|

If you need ETH2.0 spec, then use this library with `BLS_ETH=1` mode.

## Support language bindings

language|ETH2.0 spec (PublicKey = G1)|default (PublicKey = G2)|
--|--|--|
Go|[bls-eth-go-binary](https://github.com/herumi/bls-eth-go-binary)|[bls-go-binary](https://github.com/herumi/bls-go-binary)|
WebAssembly (Node.js)|[bls-eth-wasm](https://github.com/herumi/bls-eth-wasm)|[bls-wasm](https://github.com/herumi/bls-wasm)|
Rust|[bls-eth-rust](https://github.com/herumi/bls-eth-rust)|-|

## Compiled static library with `BLS_ETH=1`

The compiled static libraries with `BLS_ETH=1` mode for {windows, darwin}/amd64, linux/{amd64, arm64} and android/{arm64-v8a, armeabi-v7a}
are provided at [bls-eth-go-binary/bls/lib](https://github.com/herumi/bls-eth-go-binary/tree/master/bls/lib).

## Basic C API

### Header files

```
#define BLS_ETH
#include <mcl/bn384_256.h>
#include <bls/bls.h>
```

Remark: `BLS_ETH` must always be defined before including `bls/bls.h` if you need ETH2.0 spec mode.

### Initialization

```
// init library at once before calling the other APIs
int err = blsInit(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR);
if (err != 0) {
  printf("blsInit err %d\n", err);
  exit(1);
}

// use the latest eth2.0 spec
blsSetETHmode(BLS_ETH_MODE_LATEST);
```
Remark:
- `blsInit` and some functions which modify global settings such as `blsSetETHmode` are NOT thread-safe.
The other functions are all thread-safe.
- `blsSetETHmode` is available for only `BLS_ETH=1` mode.

### KeyGen

Init a secret key `sec` and create a public key `pub`.
```
blsSecretKey sec;
blsPublicKey pub;

// init SecretKey sec by random number
blsSecretKeySetByCSPRNG(&sec);

// get PublicKey pub from SecretKey sec
blsGetPublicKey(&pub, &sec);
```

### Sign

Make a signature `sig` of a message `msg[0..msgSize-1]` by the secret key `sec`.
```
blsSignature sig;
char msg[] = "hello";
const size_t msgSize = strlen(msg);

blsSign(&sig, &sec, msg, msgSize);
```

`msg` may contain `\x00` if the correct `msgSize` is specified.

### Verify

Verify the signature `sig` of the message `msg[0..msgSize-1]` by the public key `pub`.
```
// return 1 if it is valid else 0
int blsVerify(&sig, &pub, msg, msgSize);
```

### Aggregate Signature

Aggregate Signatures `sigVec[0]`, ..., `sigVec[n-1]` to `aggSig`.
`aggSig` is cleared if `n = 0`.

```
void blsAggregateSignature(
  blsSignature *aggSig,
  const blsSignature *sigVec,
  mclSize n
);
```

### FastAggregateVerify

Verify a signature `sig` of a message `msg[0..msgSize-1]` by `pubVec[0]`, ..., `pubVec[n-1]`.

```
int blsFastAggregateVerify(
  const blsSignature *sig,
  const blsPublicKey *pubVec,
  mclSize n,
  const void *msg,
  mclSize msgSize
);
```

### AggregateVerify

- `pubVec` is `n` array of PublicKey
- `msgVec` is `n * msgSize`-byte array, which concatenates `n`-byte messages of length `msgSize`.

Verify Signature `sig` of (Message `msgVec[msgSize * i..msgSize * (i+1)-1]` and `pubVec[i]`) for i = `0`, ..., `n-1`.

```
int blsAggregateVerifyNoCheck(
  const blsSignature *sig,
  const blsPublicKey *pubVec,
  const void *msgVec,
  mclSize msgSize,
  mclSize n
);
```

REMARK : `blsAggregateVerifyNoCheck` does not check
- `sig` has the correct order
- every `n`-byte messages of length `msgSize` are different from each other

Check them at the caller if necessary.

## Functions corresponding to ETH2.0 spec names

bls.h | eth2.0 spec name|
------|-----------------|
blsSign|Sign|
blsVerify|Verify|
blsAggregateSignature|Aggregate|
blsFastAggregateVerify|FastAggregateVerify|
blsAggregateVerifyNoCheck|AggregateVerify|

### Setter

```
int blsSecretKeySetLittleEndianMod(blsSecretKey *sec, const void *buf, mclSize bufSize);
```
Set `sec` to (`buf[0..bufSize-1]` as little endian) mod r and return 0 if `bufSize <= 64` else -1.


### Serialization

```
mclSize blsSecretKeySerialize(void *buf, mclSize maxBufSize, const blsSecretKey *sec);
mclSize blsPublicKeySerialize(void *buf, mclSize maxBufSize, const blsPublicKey *pub);
mclSize blsSignatureSerialize(void *buf, mclSize maxBufSize, const blsSignature *sig);
```
Serialize the instance to `buf[0..maxBufSize-1]` and return written byte size if success else 0.

```
mclSize blsSecretKeyDeserialize(blsSecretKey *sec, const void *buf, mclSize bufSize);
mclSize blsPublicKeyDeserialize(blsPublicKey *pub, const void *buf, mclSize bufSize);
mclSize blsSignatureDeserialize(blsSignature *sig, const void *buf, mclSize bufSize);
```
Deserialize `buf[0..bufSize-1]` to the instance and return read byte size if success else 0.


### Check order

Check whether `sig` and `pub` have the correct order `r`.

```
// return 1 if it is valid else 0
int blsSignatureIsValidOrder(const blsSignature *sig);
int blsPublicKeyIsValidOrder(const blsPublicKey *pub);
```

## API for k-of-n threshold signature

1. Prepare k secret keys (msk).
1. Make n secret keys from msk by `blsSecretKeyShare`.
1. Each user makes the public key from the given secret key.
1. Each user makes a signature for the same message.
1. Any k subset of n signatures can recover the master signature by `blsSignatureRecover`.

See [sample/minsample.c](https://github.com/herumi/bls/blob/master/sample/minsample.c#L20) for the details.

```
int blsSecretKeyShare(blsSecretKey *sec, const blsSecretKey *msk, mclSize k, const blsId *id);
```
Make `sec` corresponding to `id` from `{msk[i] for i = 0, ..., k-1}`.

```
int blsSignatureRecover(blsSignature *sig, const blsSignature *sigVec, const blsId *idVec, mclSize n);
```
Recover `sig` from `{(sigVec[i], idVec[i]) for i = 0, ..., n-1}`.

## Multi aggregate signature (experimental)

`blsMultiAggregateSignature` and `blsMultiAggregatePublicKey` are provided for [BLS Multi-Signatures With Public-Key Aggregation](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html).
The hash function is temporary.
See [blsMultiAggregateTest](https://github.com/herumi/bls/blob/master/test/bls_c_test.hpp#L356).

```
void blsMultiAggregateSignature(
  blsSignature *aggSig,
  blsSignature *sigVec,
  blsPublicKey *pubVec,
  mclSize n
);
```
Set `aggSig = sum_{i=0^n-1} sigVec[i] t_i, where (t_1, ..., t_n) = Hash({pubVec[0..n-1]})`.

```
void blsMultiAggregatePublicKey(
  blsPublicKey *aggPub,
  blsPublicKey *pubVec,
  mclSize n
);
```
Set `aggPub = sum_{i=0^n-1} pubVec[i] t_i, where (t_1, ..., t_n) = Hash({pubVec[0..n-1]})`.

## How to build a static library by ownself

The following description is for `BLS_ETH=1` mode.
Remove it if you need PublicKey as G1.

### Preliminaries

```
git clone --recursive https://github.com/herumi/bls
```

### Build static library for Linux and macOS

```
make -C mcl lib/libmcl.a
make BLS_ETH=1 lib/libbls384_256.a
```
If the option `MCL_USE_GMP=0` (resp.`MCL_USE_OPENSSL=0`) is used then GMP (resp. OpenSSL) is not used.

### Build static library for Windows

```
mklib eth
```

### Build static library for Android

See [bls-eth-go-binary](https://github.com/herumi/bls-eth-go-binary)

## History
- 2020/May/13 : `blsSetETHmode()` supports `BLS_ETH_MODE_DRAFT_07` defined at [BLS12381G2_XMD:SHA-256_SSWU_RO_](https://www.ietf.org/id/draft-irtf-cfrg-hash-to-curve-07.html#name-bls12381g2_xmdsha-256_sswu_).
- 2020/Apr/02 : *experimental* add blsMultiAggregateSignature/blsMultiAggregatePublicKey [multiSig](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html)
  - The hash function is temporary, which may be modified in the future.
- 2020/Mar/26 : DST of hash-to-curve of [mcl](https://github.com/herumi/mcl) is changed, so the output has also changed for `BLS_ETH_MODE_DRAFT_06`.
- 2020/Mar/15 : `blsSetETHmode()` supports `BLS_ETH_MODE_DRAFT_06` defined at [draft-irtf-cfrg-hash-to-curve](https://cfrg.github.io/draft-irtf-cfrg-hash-to-curve/draft-irtf-cfrg-hash-to-curve.txt) at March 2020. But it has not yet fully tested.

## License

modified new BSD License
http://opensource.org/licenses/BSD-3-Clause

## Author

MITSUNARI Shigeo(herumi@nifty.com)

## Sponsors welcome
[GitHub Sponsor](https://github.com/sponsors/herumi)
