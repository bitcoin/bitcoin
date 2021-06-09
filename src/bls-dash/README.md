# BLS Signatures implementation

![Build](https://github.com/Chia-Network/bls-signatures/workflows/Build/badge.svg)
![PyPI](https://img.shields.io/pypi/v/blspy?logo=pypi)
![PyPI - Format](https://img.shields.io/pypi/format/blspy?logo=pypi)
![GitHub](https://img.shields.io/github/license/Chia-Network/bls-signatures?logo=Github)

[![Total alerts](https://img.shields.io/lgtm/alerts/g/Chia-Network/bls-signatures.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/bls-signatures/alerts/)
[![Language grade: JavaScript](https://img.shields.io/lgtm/grade/javascript/g/Chia-Network/bls-signatures.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/bls-signatures/context:javascript)
[![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/Chia-Network/bls-signatures.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/bls-signatures/context:python)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/Chia-Network/bls-signatures.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/Chia-Network/bls-signatures/context:cpp)

NOTE: THIS LIBRARY IS NOT YET FORMALLY REVIEWED FOR SECURITY

NOTE: THIS LIBRARY WAS SHIFTED TO THE IETF BLS SPECIFICATION ON 7/16/20

Implements BLS signatures with aggregation using [relic toolkit](https://github.com/relic-toolkit/relic)
for cryptographic primitives (pairings, EC, hashing) according to the
[IETF BLS RFC](https://datatracker.ietf.org/doc/draft-irtf-cfrg-bls-signature/)
with [these curve parameters](https://datatracker.ietf.org/doc/draft-irtf-cfrg-pairing-friendly-curves/)
for BLS12-381.

Features:

* Non-interactive signature aggregation following IETF specification
* Works on Windows, Mac, Linux, BSD
* Efficient verification using Proof of Posssesion (only one pairing per distinct message)
* Aggregate public keys and private keys
* [EIP-2333](https://eips.ethereum.org/EIPS/eip-2333) key derivation (including unhardened BIP-32-like keys)
* Key and signature serialization
* Batch verification
* [Python bindings](https://github.com/Chia-Network/bls-signatures/tree/main/python-bindings)
* [Pure python bls12-381 and signatures](https://github.com/Chia-Network/bls-signatures/tree/main/python-impl)
* [JavaScript bindings](https://github.com/Chia-Network/bls-signatures/tree/main/js-bindings) (currently out of date - a great first issue!)

## Before you start

This library uses minimum public key sizes (MPL). A G2Element is a signature (96 bytes), and a G1Element is a public key (48 bytes). A private key is a 32 byte integer. There are three schemes: Basic, Augmented, and ProofOfPossession. Augmented should be enough for most use cases, and ProofOfPossession can be used where verification must be fast.

## Import the library

```c++
#include "bls.hpp"
using namespace bls;
```

## Creating keys and signatures

```c++
// Example seed, used to generate private key. Always use
// a secure RNG with sufficient entropy to generate a seed (at least 32 bytes).
vector<uint8_t> seed = {0,  50, 6,  244, 24,  199, 1,  25,  52,  88,  192,
                        19, 18, 12, 89,  6,   220, 18, 102, 58,  209, 82,
                        12, 62, 89, 110, 182, 9,   44, 20,  254, 22};

PrivateKey sk = AugSchemeMPL().KeyGen(seed);
G1Element pk = sk.GetG1Element();

vector<uint8_t> message = {1, 2, 3, 4, 5};  // Message is passed in as a byte vector
G2Element signature = AugSchemeMPL().Sign(sk, message);

// Verify the signature
bool ok = AugSchemeMPL().Verify(pk, message, signature));
```

## Serializing keys and signatures to bytes

```c++
vector<uint8_t> skBytes = sk.Serialize();
vector<uint8_t> pkBytes = pk.Serialize();
vector<uint8_t> signatureBytes = signature.Serialize();

cout << Util::HexStr(skBytes) << endl;    // 32 bytes printed in hex
cout << Util::HexStr(pkBytes) << endl;    // 48 bytes printed in hex
cout << Util::HexStr(signatureBytes) << endl;  // 96 bytes printed in hex
```

## Loading keys and signatures from bytes

```c++
// Takes vector of 32 bytes
PrivateKey skc = PrivateKey::FromByteVector(skBytes);

// Takes vector of 48 bytes
pk = G1Element::FromByteVector(pkBytes);

// Takes vector of 96 bytes
signature = G2Element::FromByteVector(signatureBytes);
```

## Create aggregate signatures

```c++
// Generate some more private keys
seed[0] = 1;
PrivateKey sk1 = AugSchemeMPL().KeyGen(seed);
seed[0] = 2;
PrivateKey sk2 = AugSchemeMPL().KeyGen(seed);
vector<uint8_t> message2 = {1, 2, 3, 4, 5, 6, 7};

// Generate first sig
G1Element pk1 = sk1.GetG1Element();
G2Element sig1 = AugSchemeMPL().Sign(sk1, message);

// Generate second sig
G1Element pk2 = sk2.GetG1Element();
G2Element sig2 = AugSchemeMPL().Sign(sk2, message2);

// Signatures can be non-interactively combined by anyone
G2Element aggSig = AugSchemeMPL().Aggregate({sig1, sig2});

ok = AugSchemeMPL().AggregateVerify({pk1, pk2}, {message, message2}, aggSig);
```

## Arbitrary trees of aggregates

```c++
seed[0] = 3;
PrivateKey sk3 = AugSchemeMPL().KeyGen(seed);
G1Element pk3 = sk3.GetG1Element();
vector<uint8_t> message3 = {100, 2, 254, 88, 90, 45, 23};
G2Element sig3 = AugSchemeMPL().Sign(sk3, message3);


G2Element aggSigFinal = AugSchemeMPL().Aggregate({aggSig, sig3});
ok = AugSchemeMPL().AggregateVerify({pk1, pk2, pk3}, {message, message2, message3}, aggSigFinal);

```

## Very fast verification with Proof of Possession scheme

```c++
// If the same message is signed, you can use Proof of Posession (PopScheme) for efficiency
// A proof of possession MUST be passed around with the PK to ensure security.

G2Element popSig1 = PopSchemeMPL().Sign(sk1, message);
G2Element popSig2 = PopSchemeMPL().Sign(sk2, message);
G2Element popSig3 = PopSchemeMPL().Sign(sk3, message);
G2Element pop1 = PopSchemeMPL().PopProve(sk1);
G2Element pop2 = PopSchemeMPL().PopProve(sk2);
G2Element pop3 = PopSchemeMPL().PopProve(sk3);

ok = PopSchemeMPL().PopVerify(pk1, pop1);
ok = PopSchemeMPL().PopVerify(pk2, pop2);
ok = PopSchemeMPL().PopVerify(pk3, pop3);
G2Element popSigAgg = PopSchemeMPL().Aggregate({popSig1, popSig2, popSig3});

ok = PopSchemeMPL().FastAggregateVerify({pk1, pk2, pk3}, message, popSigAgg);

// Aggregate public key, indistinguishable from a single public key
G1Element popAggPk = pk1 + pk2 + pk3;
ok = PopSchemeMPL().Verify(popAggPk, message, popSigAgg);

// Aggregate private keys
PrivateKey aggSk = PrivateKey::Aggregate({sk1, sk2, sk3});
ok = (PopSchemeMPL().Sign(aggSk, message) == popSigAgg);
```

## HD keys using [EIP-2333](https://github.com/ethereum/EIPs/pull/2333)

```c++
// You can derive 'child' keys from any key, to create arbitrary trees. 4 byte indeces are used.
// Hardened (more secure, but no parent pk -> child pk)
PrivateKey masterSk = AugSchemeMPL().KeyGen(seed);
PrivateKey child = AugSchemeMPL().DeriveChildSk(masterSk, 152);
PrivateKey grandChild = AugSchemeMPL().DeriveChildSk(child, 952)

// Unhardened (less secure, but can go from parent pk -> child pk), BIP32 style
G1Element masterPk = masterSk.GetG1Element();
PrivateKey childU = AugSchemeMPL().DeriveChildSkUnhardened(masterSk, 22);
PrivateKey grandchildU = AugSchemeMPL().DeriveChildSkUnhardened(childU, 0);

G1Element childUPk = AugSchemeMPL().DeriveChildPkUnhardened(masterPk, 22);
G1Element grandchildUPk = AugSchemeMPL().DeriveChildPkUnhardened(childUPk, 0);

ok = (grandchildUPk == grandchildU.GetG1Element();
```

## Build

Cmake 3.14+, a c++ compiler, and python3 (for bindings) are required for building.

```bash
mkdir build
cd build
cmake ../
cmake --build . -- -j 6
```

### Run tests

```bash
./build/src/runtest
```

### Run benchmarks

```bash
./build/src/runbench
```

On a 3.5 GHz i7 Mac, verification takes about 1.1ms per signature, and signing takes 1.3ms.

### Link the library to use it

```bash
g++ -Wl,-no_pie -std=c++11  -Ibls-signatures/build/_deps/relic-src/include -Ibls-signatures/build/_deps/relic-build/include -Ibls-signatures/src -L./bls-signatures/build/ -l bls yourapp.cpp
```

## Notes on dependencies

Libsodium and GMP are optional dependencies: libsodium gives secure memory
allocation, and GMP speeds up the library by ~ 3x. MPIR is used on Windows via
GitHub Actions instead. To install them, either download them from github and
follow the instructions for each repo, or use a package manager like APT or
brew. You can follow the recipe used to build python wheels for multiple
platforms in `.github/workflows/`. libsodium is dynamically linked unless
the environment variable $CIBUILDWHEEL is set which will then cause
libsodium to statically link.

## Discussion

Discussion about this library and other Chia related development is in the #dev
channel of Chia's [public Keybase channels](https://keybase.io/team/chia_network.public).

## Code style

* Always use vector<uint8_t> for bytes
* Use size_t for size variables
* Uppercase method names
* Prefer static constructors
* Avoid using templates
* Objects allocate and free their own memory
* Use cpplint with default rules
* Use SecAlloc and SecFree when handling secrets

## ci Building

The primary build process for this repository is to use GitHub Actions to
build binary wheels for MacOS, Linux (x64 and aarch64), and Windows and publish
them with a source wheel on PyPi. MacOS ARM64 is supported but not automated
due to a lack of M1 CI runners. See `.github/workflows/build.yml`. CMake uses
[FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
to download [pybind11](https://github.com/pybind/pybind11) for the Python
bindings and relic from a chia relic forked repository for Windows. Building
is then managed by [cibuildwheel](https://github.com/joerick/cibuildwheel).
Further installation is then available via `pip install blspy` e.g. The ci
builds include GMP and a statically linked libsodium.

## Contributing and workflow

Contributions are welcome and more details are available in chia-blockchain's
[CONTRIBUTING.md](https://github.com/Chia-Network/chia-blockchain/blob/main/CONTRIBUTING.md).

The main branch is usually the currently released latest version on PyPI.
Note that at times bls-signatures/blspy will be ahead of the release version
that chia-blockchain requires in it's main/release version in preparation
for a new chia-blockchain release. Please branch or fork main and then create
a pull request to the main branch. Linear merging is enforced on main and
merging requires a completed review. PRs will kick off a GitHub actions ci
build and analysis of bls-signatures at
[lgtm.com](https://lgtm.com/projects/g/Chia-Network/bls-signatures/?mode=list).
Please make sure your build is passing and that it does not increase alerts
at lgtm.

## Specification and test vectors

The [IETF bls draft](https://datatracker.ietf.org/doc/draft-irtf-cfrg-hash-to-curve/)
is followed. Test vectors can also be seen in the python and cpp test files.

## Libsodium license

The libsodium static library is licensed under the ISC license which requires
the following copyright notice.

>ISC License
>
>Copyright (c) 2013-2020
>Frank Denis \<j at pureftpd dot org\>
>
>Permission to use, copy, modify, and/or distribute this software for any
>purpose with or without fee is hereby granted, provided that the above
>copyright notice and this permission notice appear in all copies.
>
>THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
>WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
>MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
>ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
>WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
>ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
>OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

## GMP license

GMP is distributed under the
[GNU LGPL v3 license](https://www.gnu.org/licenses/lgpl-3.0.html)

## Relic license

Relic is used with the
[Apache 2.0 license](https://github.com/relic-toolkit/relic/blob/master/LICENSE.Apache-2.0)
