# PIP-2030 Quantum Shield (ML-DSA Mode 3)

A standalone experimental toolset implementing the NIST-standardized
ML-DSA-65 post-quantum signature scheme (CRYSTALS-Dilithium, Mode 3
parameter set). It serves as a foundation for experimentation toward
post-quantum signature outputs in Bitcoin, addressing the existential
threat that a cryptographically relevant quantum computer poses to
ECDSA/Schnorr signatures on secp256k1.

> **Status:** Experimental, additive-only tool in `contrib/`. No consensus
> changes. ML-DSA is the FIPS 204 standardized signature scheme, the
> successor of CRYSTALS-Dilithium Mode 3.

## Building

The toolset requires [liboqs](https://github.com/open-quantum-safe/liboqs)
with the ML-DSA algorithm family enabled:

```sh
git clone --depth 1 https://github.com/open-quantum-safe/liboqs
cmake -S liboqs -B liboqs/build -DOQS_BUILD_ONLY_LIB=ON \
      -DOQS_MINIMAL_BUILD="Sig:Ml-Dsa*"
cmake --build liboqs/build
cmake -S contrib/pq-shield -B build/pq-shield \
      -DLIBOQS_ROOT=$PWD/liboqs/build
cmake --build build/pq-shield
```

## Usage

```sh
pq-shield keygen <dir>     # writes <dir>/pq.pub and <dir>/pq.key (0600)
pq-shield sign <key> <msg> <out.sig>
pq-shield verify <pub> <msg> <sig>
pq-shield-audit <path>     # validates key sizes and file permissions
```

## Security notes

- Private key files are created with mode `0600`; the audit tool refuses
  keys with looser permissions.
- ML-DSA-65 provides NIST security level 3 (approximately AES-192).
- Signature size is ~3.3 KB, public key ~1.9 KB — roughly 10-20x larger
  than ECDSA. Consensus integration will require a dedicated output type
  and size accounting.
