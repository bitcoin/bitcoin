# Python bindings

Use the full power and efficiency of the C++ bls library, but in a few lines of python!

## Install

```bash
pip3 install blspy

```

Alternatively, to install from source, run the following, in the project root directory:

```bash
pip3 install .
```

Cmake, a c++ compiler, and a recent version of pip3 (v18) are required for source install.
GMP(speed) is an optional dependency.
Public keys are G1Elements, and signatures are G2Elements.

Then, to use:

## Import the library

```python
from blspy import (PrivateKey, Util, AugSchemeMPL, PopSchemeMPL,
                   G1Element, G2Element)
```

## Creating keys and signatures

```python
# Example seed, used to generate private key. Always use
# a secure RNG with sufficient entropy to generate a seed (at least 32 bytes).
seed: bytes = bytes([0,  50, 6,  244, 24,  199, 1,  25,  52,  88,  192,
                        19, 18, 12, 89,  6,   220, 18, 102, 58,  209, 82,
                        12, 62, 89, 110, 182, 9,   44, 20,  254, 22])
sk: PrivateKey = AugSchemeMPL.key_gen(seed)
pk: G1Element = sk.get_g1()

message: bytes = bytes([1, 2, 3, 4, 5])
signature: G2Element = AugSchemeMPL.sign(sk, message)

# Verify the signature
ok: bool = AugSchemeMPL.verify(pk, message, signature)
assert ok
```

## Serializing keys and signatures to bytes

```python
sk_bytes: bytes = bytes(sk)  # 32 bytes
pk_bytes: bytes = bytes(pk)  # 48 bytes
signature_bytes: bytes = bytes(signature)  # 96 bytes

print(sk_bytes.hex(), pk_bytes.hex(), signature_bytes.hex())
```

## Loading keys and signatures from bytes

```python
sk = PrivateKey.from_bytes(sk_bytes)
pk = G1Element.from_bytes(pk_bytes)
signature = G2Element.from_bytes(signature_bytes)
```

## Create aggregate signatures

```python
# Generate some more private keys
seed = bytes([1]) + seed[1:]
sk1: PrivateKey = AugSchemeMPL.key_gen(seed)
seed = bytes([2]) + seed[1:]
sk2: PrivateKey = AugSchemeMPL.key_gen(seed)
message2: bytes = bytes([1, 2, 3, 4, 5, 6, 7])

# Generate first sig
pk1: G1Element = sk1.get_g1()
sig1: G2Element = AugSchemeMPL.sign(sk1, message)

# Generate second sig
pk2: G1Element = sk2.get_g1()
sig2: G2Element = AugSchemeMPL.sign(sk2, message2)

# Signatures can be non-interactively combined by anyone
agg_sig: G2Element = AugSchemeMPL.aggregate([sig1, sig2])

ok = AugSchemeMPL.aggregate_verify([pk1, pk2], [message, message2], agg_sig)
```

## Arbitrary trees of aggregates

```python
seed = bytes([3]) + seed[1:]
sk3: PrivateKey = AugSchemeMPL.key_gen(seed)
pk3: G1Element = sk3.get_g1()
message3: bytes = bytes([100, 2, 254, 88, 90, 45, 23])
sig3: G2Element = AugSchemeMPL.sign(sk3, message3)

agg_sig_final: G2Element = AugSchemeMPL.aggregate([agg_sig, sig3])
ok = AugSchemeMPL.aggregate_verify([pk1, pk2, pk3], [message, message2, message3], agg_sig_final)
```

## Very fast verification with Proof of Possession scheme

```python
# If the same message is signed, you can use Proof of Posession (PopScheme) for efficiency
# A proof of possession MUST be passed around with the PK to ensure security.
pop_sig1: G2Element = PopSchemeMPL.sign(sk1, message)
pop_sig2: G2Element = PopSchemeMPL.sign(sk2, message)
pop_sig3: G2Element = PopSchemeMPL.sign(sk3, message)
pop1: G2Element = PopSchemeMPL.pop_prove(sk1)
pop2: G2Element = PopSchemeMPL.pop_prove(sk2)
pop3: G2Element = PopSchemeMPL.pop_prove(sk3)

ok = PopSchemeMPL.pop_verify(pk1, pop1)
ok = PopSchemeMPL.pop_verify(pk2, pop2)
ok = PopSchemeMPL.pop_verify(pk3, pop3)

pop_sig_agg: G2Element = PopSchemeMPL.aggregate([pop_sig1, pop_sig2, pop_sig3])

ok = PopSchemeMPL.fast_aggregate_verify([pk1, pk2, pk3], message, pop_sig_agg)

# Aggregate public key, indistinguishable from a single public key
pop_agg_pk: G1Element = pk1 + pk2 + pk3
ok = PopSchemeMPL.verify(pop_agg_pk, message, pop_sig_agg)

# Aggregate private keys
pop_agg_sk: PrivateKey = PrivateKey.aggregate([sk1, sk2, sk3])
ok = PopSchemeMPL.sign(pop_agg_sk, message) == pop_sig_agg
```

## HD keys using [EIP-2333](https://github.com/ethereum/EIPs/pull/2333)

```python
master_sk: PrivateKey = AugSchemeMPL.key_gen(seed)
child: PrivateKey = AugSchemeMPL.derive_child_sk(master_sk, 152)
grandchild: PrivateKey = AugSchemeMPL.derive_child_sk(child, 952)

master_pk: G1Element = master_sk.get_g1()
child_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(master_sk, 22)
grandchild_u: PrivateKey = AugSchemeMPL.derive_child_sk_unhardened(child_u, 0)

child_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(master_pk, 22)
grandchild_u_pk: G1Element = AugSchemeMPL.derive_child_pk_unhardened(child_u_pk, 0)

ok = (grandchild_u_pk == grandchild_u.get_g1())
```
