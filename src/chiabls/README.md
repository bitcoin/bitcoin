### BLS Signatures implementation
[![Build Status](https://travis-ci.org/Chia-Network/bls-signatures.svg?branch=master)](https://travis-ci.org/Chia-Network/bls-signatures)

NOTE: THIS LIBRARY IS A DRAFT AND NOT YET REVIEWED FOR SECURITY

Implements BLS signatures with aggregation as in [Boneh, Drijvers, Neven 2018](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html), using
[relic toolkit](https://github.com/relic-toolkit/relic) for cryptographic primitives (pairings, EC, hashing).
The [BLS12-381](https://github.com/zkcrypto/pairing/tree/master/src/bls12_381) curve is used. The spec is [here](https://github.com/Chia-Network/bls-signatures/tree/master/SPEC.md).

Features:
* Non-interactive signature aggregation on identical or distinct messages
* Aggregate aggregates (trees)
* Efficient verification (only one pairing per distinct message)
* Security against rogue public key attack
* Aggregate public keys and private keys
* HD (BIP32) key derivation
* Key and signature serialization
* Batch verification
* Signature division (divide an aggregate by a previously verified signature)
* [Python bindings](https://github.com/Chia-Network/bls-signatures/tree/master/python-bindings)
* [Pure python bls12-381 and signatures](https://github.com/Chia-Network/bls-signatures/tree/master/python-impl)


#### Import the library
```c++
#include "bls.hpp"
```

#### Creating keys and signatures
```c++
// Example seed, used to generate private key. Always use
// a secure RNG with sufficient entropy to generate a seed.
uint8_t seed[] = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                  19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                  82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};

bls::PrivateKey sk = bls::PrivateKey::FromSeed(seed, sizeof(seed));
bls::PublicKey pk = sk.GetPublicKey();

uint8_t msg[] = {100, 2, 254, 88, 90, 45, 23};

bls::Signature sig = sk.Sign(msg, sizeof(msg));
```

#### Serializing keys and signatures to bytes
```c++
uint8_t skBytes[bls::PrivateKey::PRIVATE_KEY_SIZE];  // 32 byte array
uint8_t pkBytes[bls::PublicKey::PUBLIC_KEY_SIZE];    // 48 byte array
uint8_t sigBytes[bls::Signature::SIGNATURE_SIZE];    // 96 byte array

sk.Serialize(skBytes);   // 32 bytes
pk.Serialize(pkBytes);   // 48 bytes
sig.Serialize(sigBytes); // 96 bytes
```

#### Loading keys and signatures from bytes
```c++
// Takes array of 32 bytes
sk = bls::PrivateKey::FromBytes(skBytes);

// Takes array of 48 bytes
pk = bls::PublicKey::FromBytes(pkBytes);

// Takes array of 96 bytes
sig = bls::Signature::FromBytes(sigBytes);
```

#### Verifying signatures
```c++
// Add information required for verification, to sig object
sig.SetAggregationInfo(bls::AggregationInfo::FromMsg(pk, msg, sizeof(msg)));

bool ok = sig.Verify();
```

#### Aggregate signatures for a single message
```c++
// Generate some more private keys
seed[0] = 1;
bls::PrivateKey sk1 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
seed[0] = 2;
bls::PrivateKey sk2 = bls::PrivateKey::FromSeed(seed, sizeof(seed));

// Generate first sig
bls::PublicKey pk1 = sk1.GetPublicKey();
bls::Signature sig1 = sk1.Sign(msg, sizeof(msg));

// Generate second sig
bls::PublicKey pk2 = sk2.GetPublicKey();
bls::Signature sig2 = sk2.Sign(msg, sizeof(msg));

// Aggregate signatures together
vector<bls::Signature> sigs = {sig1, sig2};
bls::Signature aggSig = bls::Signature::Aggregate(sigs);

// For same message, public keys can be aggregated into one.
// The signature can be verified the same as a single signature,
// using this public key.
vector<bls::PublicKey> pubKeys = {pk1, pk2};
bls::PublicKey aggPubKey = bls::Signature::Aggregate(pubKeys);
```

#### Aggregate signatures for different messages
```c++
// Generate one more key and message
seed[0] = 3;
bls::PrivateKey sk3 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
bls::PublicKey pk3 = sk3.GetPublicKey();
uint8_t msg2[] = {100, 2, 254, 88, 90, 45, 23};

// Generate the signatures, assuming we have 3 private keys
sig1 = sk1.Sign(msg, sizeof(msg));
sig2 = sk2.Sign(msg, sizeof(msg));
bls::Signature sig3 = sk3.Sign(msg2, sizeof(msg2));

// They can be noninteractively combined by anyone
// Aggregation below can also be done by the verifier, to
// make batch verification more efficient
vector<bls::Signature> sigsL = {sig1, sig2};
bls::Signature aggSigL = bls::Signature::Aggregate(sigsL);

// Arbitrary trees of aggregates
vector<bls::Signature> sigsFinal = {aggSigL, sig3};
bls::Signature aggSigFinal = bls::Signature::Aggregate(sigsFinal);

// Serialize the final signature
aggSigFinal.Serialize(sigBytes);
```

#### Verify aggregate signature for different messages
```c++
// Deserialize aggregate signature
aggSigFinal = bls::Signature::FromBytes(sigBytes);

// Create aggregation information (or deserialize it)
bls::AggregationInfo a1 = bls::AggregationInfo::FromMsg(pk1, msg, sizeof(msg));
bls::AggregationInfo a2 = bls::AggregationInfo::FromMsg(pk2, msg, sizeof(msg));
bls::AggregationInfo a3 = bls::AggregationInfo::FromMsg(pk3, msg2, sizeof(msg2));
vector<bls::AggregationInfo> infos = {a1, a2};
bls::AggregationInfo a1a2 = bls::AggregationInfo::MergeInfos(infos);
vector<bls::AggregationInfo> infos2 = {a1a2, a3};
bls::AggregationInfo aFinal = bls::AggregationInfo::MergeInfos(infos2);

// Verify final signature using the aggregation info
aggSigFinal.SetAggregationInfo(aFinal);
ok = aggSigFinal.Verify();

// If you previously verified a signature, you can also divide
// the aggregate signature by the signature you already verified.
ok = aggSigL.Verify();
vector<bls::Signature> cache = {aggSigL};
aggSigFinal = aggSigFinal.DivideBy(cache);

// Final verification is now more efficient
ok = aggSigFinal.Verify();
```

#### Aggregate private keys
```c++
vector<bls::PrivateKey> privateKeysList = {sk1, sk2};
vector<bls::PublicKey> pubKeysList = {pk1, pk2};

// Create an aggregate private key, that can generate
// aggregate signatures
const bls::PrivateKey aggSk = bls::PrivateKey::Aggregate(
        privateKeys, pubKeys);

bls::Signature aggSig3 = aggSk.Sign(msg, sizeof(msg));
```

#### HD keys
```c++
// Random seed, used to generate master extended private key
uint8_t seed[] = {1, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                  19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                  82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};

bls::ExtendedPrivateKey esk = bls::ExtendedPrivateKey::FromSeed(
        seed, sizeof(seed));

bls::ExtendedPublicKey epk = esk.GetExtendedPublicKey();

// Use i >= 2^31 for hardened keys
bls::ExtendedPrivateKey skChild = esk.PrivateChild(0)
                                .PrivateChild(5);

bls::ExtendedPublicKey pkChild = epk.PublicChild(0)
                               .PublicChild(5);

// Serialize extended keys
uint8_t buffer1[bls::ExtendedPublicKey::ExtendedPublicKeySize]   // 93 bytes
uint8_t buffer2[bls::ExtendedPrivateKey::ExtendedPrivateKeySize] // 77 bytes

pkChild.Serialize(buffer1);
skChild.Serialize(buffer2);
```

### Build
Cmake, a c++ compiler, and python3 (for bindings) are required for building.
```bash
git submodule update --init --recursive
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

### Link the library to use it
```bash
g++ -Wl,-no_pie  -Ibls-signatures/contrib/relic/include -Ibls-signatures/build/contrib/relic/incl
ude -Ibls-signatures/src/  -L./bls-signatures/build/ -l bls  yourfile.cpp
```

### Notes on dependencies
Changes performed to relic: Added config files for Chia, and added gmp include in relic.h, new ep_map and ep2_map, new ep_pck and ep2_pck. Custom inversion function. Note: relic is used with the Apache 2.0 license.

Libsodium and GMP are optional dependencies: libsodium gives secure memory allocation,
and GMP speeds up the library by ~ 3x. To install them, either download them from github
and follow the instructions for each repo, or use a package manager like APT or brew.

### Discussion
Discussion about this library and other Chia related development is on Keybase.
Install Keybase, and run the following to join the Chia public channels:

```bash
keybase team request-access chia_network.public
```

### Code style
* Always use uint8_t for bytes
* Use size_t for size variables
* Uppercase method names
* Prefer static constructors
* Avoid using templates
* Objects allocate and free their own memory
* Use cpplint with default rules

### TODO
* Serialize aggregation info
* Secure allocation during signing, key derivation
* Threshold signatures
* Remove unnecessary dependency files
* Constant time and side channel attacks
* Adaptor signatures / Blind signatures
* More tests vectors (failed verifications, etc)


### Specification and test vectors
The specification and test vectors can be found [here](https://github.com/Chia-Network/bls-signatures/tree/master/SPEC.md). Test vectors can also be seen in the python or cpp test files.