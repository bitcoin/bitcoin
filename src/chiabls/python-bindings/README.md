## Python bindings
Install
```bash
pip3 install blspy
```
Cmake, a c++ compiler, and a recent version of pip3 (v18) are required.
GMP(speed) and libsodium(secure memory alloc) are optional dependencies.

To install from source, run the following, in the project root directory:

```bash
git submodule init
git submodule update
pip3 install .
```

Then, to use:

#### Import the library
```python
import blspy
```

#### Creating keys and signatures
```python
# Example seed, used to generate private key. Always use
# a secure RNG with sufficient entropy to generate a seed.
seed = bytes([0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22])
sk = PrivateKey.from_seed(seed)
pk = sk.get_public_key()

msg = bytes([100, 2, 254, 88, 90, 45, 23])

sig = sk.sign(msg)
```

#### Serializing keys and signatures to bytes
```python
sk_bytes = sk.serialize() # 32 bytes
pk_bytes = pk.serialize() # 48 bytes
sig_bytes = sig.serialize() # 96 bytes
```

#### Loading keys and signatures from bytes
```python
# Takes bytes object of size 32
sk = PrivateKey.from_bytes(sk_bytes)
# Takes bytes object of size 48
pk = PublicKey.from_bytes(pk_bytes)
# Takes bytes object of size 96
sig = Signature.from_bytes(sig_bytes)
```

#### Verifying signatures
```python
# Add information required for verification, to sig object
sig.set_aggregation_info(AggregationInfo.from_msg(pk, msg))
ok = BLS.verify(sig)
```

#### Aggregate signatures for a single message
```python
# Generate some more private keys
seed = bytes([1]) + seed[1:]
sk1 = PrivateKey.from_seed(seed)
seed = bytes([2]) + seed[1:]
sk2 = PrivateKey.from_seed(seed)

# Generate first sig
pk1 = sk1.get_public_key()
sig1 = sk1.sign(msg)

# Generate second sig
pk2 = sk2.get_public_key()
sig2 = sk2.sign(msg)

# Aggregate signatures together
agg_sig = BLS.aggregate_sigs([sig1, sig2])


# For same message, public keys can be aggregated into one.
# The signature can be verified the same as a single signature,
# using this public key.
agg_pubkey = BLS.aggregate_pub_keys([pk1, pk2], True)
```

#### Aggregate signatures for different messages
```python
# Generate one more key and message
seed = bytes([3]) + seed[1:]
sk3 = PrivateKey.from_seed(seed)
pk3 = sk3.get_public_key()
msg2 = bytes([100, 2, 254, 88, 90, 45, 23])

# Generate the signatures, assuming we have 3 private keys
sig1 = sk1.sign(msg)
sig2 = sk2.sign(msg)
sig3 = sk3.sign(msg2)

# They can be noninteractively combined by anyone
# Aggregation below can also be done by the verifier, to
# make batch verification more efficient
agg_sig_l = BLS.aggregate_sigs([sig1, sig2])

# Arbitrary trees of aggregates
agg_sig_final = BLS.aggregate_sigs([agg_sig_l, sig3])

# Serialize the final signature
sig_bytes = agg_sig_final.serialize()
```

#### Verify aggregate signature for different messages
```python
# Deserialize aggregate signature
agg_sig_final = Signature.from_bytes(sig_bytes)

# Create aggregation information (or deserialize it)
a1 = AggregationInfo.from_msg(pk1, msg)
a2 = AggregationInfo.from_msg(pk2, msg)
a3 = AggregationInfo.from_msg(pk3, msg2)
a1a2 = AggregationInfo.merge_infos([a1, a2])
a_final = AggregationInfo.merge_infos([a1a2, a3])

# Verify final signature using the aggregation info
agg_sig_final.set_aggregation_info(a_final)
ok = BLS.verify(agg_sig_final)

# If you previously verified a signature, you can also divide
# the aggregate signature by the signature you already verified.
ok = BLS.verify(agg_sig_l)
agg_sig_final = agg_sig_final.divide_by([agg_sig_l])

ok = BLS.verify(agg_sig_final)
```

#### Aggregate private keys
```python
# Create an aggregate private key, that can generate
# aggregate signatures
agg_sk = BLS.aggregate_priv_keys([sk1, sk2], [pk1, pk2], True)
agg_sk.sign(msg)
```

#### HD keys
```python
# Random seed, used to generate master extended private key
seed = bytes([1, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22])

esk = ExtendedPrivateKey.from_seed(seed)
epk = esk.get_extended_public_key()

sk_child = esk.private_child(0).private_child(5)
pk_child = epk.public_child(0).public_child(5)

buffer1 = pk_child.serialize() # 93 bytes
buffer2 = sk_child.serialize() # 77 bytes
```

