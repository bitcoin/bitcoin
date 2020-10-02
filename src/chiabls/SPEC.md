# BLS Signature Scheme Specificiation

This document describes an instantiation of the BLS Signature Scheme as it will be used in the Chia blockchain. It still a draft and subject to change.

The curve used is BLS12-381 as described [here](https://github.com/ebfull/pairing/tree/master/src/bls12_381). This spec is based off of zkcrypto/pairing spec, described in that link. The aggregation used is based on [Boneh, Drijvers, Neven](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html), with additional aggregation of aggregates, and signature division.

Multiplicative notation is used for all groups. G1 is used for public keys, and G2 is used for signatures. There are several other small differences with the rust pairing spec.

## Parameters

```python
# BLS parameter, used to generate other parameters
x = -0xd201000000010000

# 381 bit prime defining the field Fq. q = (x - 1)2 ((x4 - x2 + 1) / 3) + x
q = 0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab

# Elliptic curve, G1 is the r-order subgroup of points on this curve
E: y^2 = x^3 + 4

# Twist of E, x and y are elements of Fq^2. G2 is the r-order subgroup of points this curve
"E'": y^2 = x^3 + 4(i+1)

# Generator for G1, consisting of x and y coordinates
gx = (0x17F1D3A73197D7942695638C4FA9AC0FC3688C4F9774B905A14E3A3F171BAC586C55E83FF97A1AEFFB3AF00ADB22C6BB)
gy = (0x08B3F481E3AAA0F1A09E30ED741D8AE4FCF5E095D5D00AF600DB18CB2C04B3EDD03CC744A2888AE40CAA232946C5E7E1)

# Generator for G2, consisting of x and y coordinates
g2x = (0x24aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb8, 0x13e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e)
g2y =
(0xce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801, 0x606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be)

# Order of G1, G2, and GT. r = (x4 - x2 + 1)
r = n = 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001

# Cofactor by which to multiply points to map them to G1. (on to the r-torsion). h = (x - 1)2 / 3
h = 0x396C8C005555E1568C00AAAB0000AAAB

# Embedding degree of curve
k = 12

# Extension towers
Fq2: Fq(u) / (u2 - β) where β = -1.
Fq6: Fq2(v) / (v3 - ξ) where ξ = u + 1
Fq12: Fq6(w) / (w2 - γ) where γ = v
```

## Subroutines

### Pairing operation e
Performs an ate pairing between p and q, up to x.
* input: g<sub>1</sub> element p, g<sub>2</sub> element q
* output: Fq12 element


### swEncode
Shallue and van de Woestijne encoding of a field element to a curve point. Used for Fouque-Tibouchi hashing: https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf. 0 maps to the point at infinity.
* input: Fq element
* output: G<sub>1</sub> or G<sub>2</sub> element


### psi
Endormophism used to speed up cofactor multiplications in hashG<sub>2</sub>.
* input: G<sub>1</sub> element p
* output: G<sub>2</sub> element p2
```python
# Twist is map from E -> E'
# Untwist is map From E' -> E
# Described in page 4 of "Efficient hash maps to G2 on BLS curves" by Budroni and Pintore.
return twist(qPowerFrobenius(untwist(p)))
```

### hash256
Hash function with 256 bit outputs.
* input: message m
* output: 256 bit bytearray
```python
return SHA256(m)
```

### hash512
Hash function with 512 bit outputs.
* input: message m
* output: 512 bit bytearray
```python
# 0 or 1 bytes are appended to each input
return hash256(m + 0) + hash256(m + 1)
```

### hashG<sub>1</sub>
Maps any string to a deterministic random point in G<sub>1</sub>.
* input: message m
* output: G<sub>1</sub> element
```python
h <- hash256(m)
t0 <- hash512(h + b"G1_0") % q
t1 <- hash512(h + b"G1_1") % q

p <- swEncode(t0) * swEncode(t1)

# Map to the r-torsion by raising to cofactor power
return p ^ h
```

### hashG<sub>2</sub>
Maps any string to a deterministic random point in G<sub>2</sub>.
* input: message m
* output: G<sub>2</sub> element
```python
h <- hash256(m)
t00 <- hash512(h + b"G2_0_c0") % q
t01 <- hash512(h + b"G2_0_c1") % q
t10 <- hash512(h + b"G2_1_c0") % q
t11 <- hash512(h + b"G2_1_c1") % q

t0 <- Fq2(t00, t01)
t1 <- Fq2(t10, t11)

p <- swEncode(t0) * swEncode(t1)

# Map to the r-torsion by raising to cofactor power
# Described in page 11 of "Efficient hash maps to G2 on BLS curves" by Budroni and Pintore.
x <- abs(x)
return p ^ (x^2 + x - 1) - psi(p ^ (x + 1)) + psi(psi(p ^ 2))
```

### hashPks
Maps a set of public keys into a list of m values.
* input: G<sub>1</sub> elements pks, number of outputs m
* output: n 256bit integers T
```python
pkHash <- hash256(pk.serialize() for pk in sorted pks)
return [(hash256(fourBytes(i) + pkHash) % n) for i in range(m)]
```

## Methods
### keyGen
Creates a public/private keypair, using a seed s. Private keys are 255 bit integers, and public keys are G<sub>1</sub> elements.
* input: random seed s
* output: field element in Z<sub>q</sub>, G<sub>1</sub> element
```python
# Perform an HMAC using hash256, and the following string as the key
sk <- hmac256(s, b"BLS private key seed") mod n
pk <- g1 ^ sk
```

### sign
Signs a message m with private key sk.
* input: bytes m, Z<sub>q</sub> element sk
* output: G<sub>2</sub> element σ
```python
σ <- hashG2(m) ^ sk
```

### verify
Verifies that a signature is valid, for a collection of public keys, messages, and exponents (aggInfo).
* input:
    * G<sub>2</sub> element σ
    * map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) aggInfo
* output: bool
```python
if aggInfo is empty: return false
pks = []
ms = []
for each distinct messsageHash m in aggInfo:
    pkAgg <- 1
    for each pk grouped with m:
        pkAgg *= pk ^ aggInfo[(m, pk)]
    pks.add(pkAgg)
    ms.add(m)
return 1 == e(g1 ^ (q-1), σ) * prod e(pks[i], ms[i])
```

### aggregate
Aggregates multiple aggregate signatures into one signature. This also takes in an aggregationInfo object for each signature, and aggregates these into one. Simple or secure aggregation is used, depending on whether messages are all distinct or not.
* input:
    * list of G<sub>2</sub> elements: signatures
    * list of map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) aggInfo
* output:
    * G<sub>2</sub> element σ<sub>agg</sub>
    * map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) newAggInfo
```python
messageHashes <- [i.messageHashes for i in aggInfo]
publicKeys <- [i.pks for i in aggInfo]

collidingMessages <- messages that appear in more than one messageHashes list

if colidingMessages is empty:
    # If all messages are distinct, use simple aggregation
    sigAgg <- product([sig for sig in signatures])
    newAggInfo <- mergeInfos(aggInfo)
    return sigAgg, newAggInfo

collidingSigs <- signatures that contain collidingMessages
nonCollidingSigs <- remaining signatures
sortKeys <- all (message, publicKey) pairs in colliding groups

sort(sort_keys) # Sort first by message, then by pk
sortedPublicKeys <- [k[1] for k in sortKeys]
Ts <- hashPks(sortedPublicKeys, len(collidingSigs))

sigAgg <- product(collidingSigs[i] ^ Ts[i] for i in collidingSigs) * product(sig for sig in nonCollidingSigs)
newAggInfo = mergeInfos(aggInfos)
return (sigAgg, newAggInfo)
```

### divide
Divides one signature by a list of other signatures. This removes them from the dividend signature, so that verifying the resulting signature, does not verify any of the divisor signatures. This is useful for optimizing the verification speed of an aggregate signature.
* input:
    * Divident signature: dividendSig
    * map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) dividendAggInfo
    * Divisor signatures: divisorSigs
    * list of map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) divisorsAggInfo
* output:
    * G<sub>2</sub> element σ<sub>agg</sub>
    * map((bytes m, G<sub>1</sub> pk) -> Z<sub>n</sub> exponent) newAggInfo
```python
messageHashesToRemove <- []
pubKeysToRemove <- []
prod <- 1 # Point at infinity
for divisorSig, i in enumerate(divisorSigs):
    for j in range(len(divisorsAggInfo[i].keys)):
        divisor <- divisorsAggInfo[i][j]
        assert(j in dividendAggInfo[i])
        dividend <- dividendAggInfo[i][j]
        if j == 0:
            quotient <- divided / divisor in Fq
        else:
            assert((divided / divisor in Fq) == quotient)
        messageHashesToRemove.append(divisorSig.messageHashes[j])
        pubKeysToRemove.append(divisorSig.pubkeys[j])
    prod <- prod * -divisorSig
aggSig <- dividendSig * prod
newAggInfo <- dividendAggInfo
newAggInfo.remove((messageHashesToRemove[i], pubKeysToRemove[i] for i in range(len(messageHashesToRemove)))
return (aggSig, newAggInfo)
```

## Serialization
**private key (32 bytes):** Big endian integer.

**pubkey (48 bytes):** 381 bit affine x coordinate, encoded into 48 big-endian bytes. Since we have 3 bits left over in the beginning, the first bit is set to 1 iff y coordinate is the lexicographically largest of the two valid ys. The public key fingerprint is the first 4 bytes of hash256(serialize(pubkey)).

**signature (96 bytes):** Two 381 bit integers (affine x coordinate), encoded into two 48 big-endian byte arrays. Since we have 3 bits left over in the beginning, the first bit is set to 1 iff the y coordinate is the lexicographically largest of the two valid ys. (The term with the i is compared first, i.e 3i + 1 > 2i + 7).


## HD keys
HD (Hierarchical Deterministic) keys allow deriving many public and private keys from one seed, and even deriving public keys from other public keys.

HD keys follow Bitcoin's BIP32 specification, with the following differences:
* The HMAC key to generate a master private key used is not "Bitcoin seed" it is "BLS HD seed".
* The master secret key is generated mod n from the master seed,
since not all 32 byte sequences are valid BLS private keys
* Instead of SHA512(input), do hash512(input) as defined above.
* Mod n for the output of key derivation.
* ID of a key is hash256(pk) instead of HASH160(pk)
* Serialization of extended public key is 93 bytes, since BLS public keys are longer

## Test vectors
### Signatures
* keygen([1,2,3,4,5])
    * sk1: 0x022fb42c08c12de3a6af053880199806532e79515f94e83461612101f9412f9e
    * pk1 fingerprint: 0x26d53247
* keygen([1,2,3,4,5,6])
    * pk2 fingerprint: 0x289bb56e
* sign([7,8,9], sk1)
    * sig1:  0x93eb2e1cb5efcfb31f2c08b235e8203a67265bc6a13d9f0ab77727293b74a357ff0459ac210dc851fcb8a60cb7d393a419915cfcf83908ddbeac32039aaa3e8fea82efcb3ba4f740f20c76df5e97109b57370ae32d9b70d256a98942e5806065
* sign([7,8,9], sk2)
    * sig2: 0x975b5daa64b915be19b5ac6d47bc1c2fc832d2fb8ca3e95c4805d8216f95cf2bdbb36cc23645f52040e381550727db420b523b57d494959e0e8c0c6060c46cf173872897f14d43b2ac2aec52fc7b46c02c5699ff7a10beba24d3ced4e89c821e
* verify(sig1, AggregationInfo(pk1, [7,8,9]))
    * true
* verify(sig2, AggregationInfo(pk2, [7,8,9]))
    * true

### Aggregation
* aggregate([sig1, sig2])
    * aggSig: 0x975b5daa64b915be19b5ac6d47bc1c2fc832d2fb8ca3e95c4805d8216f95cf2bdbb36cc23645f52040e381550727db420b523b57d494959e0e8c0c6060c46cf173872897f14d43b2ac2aec52fc7b46c02c5699ff7a10beba24d3ced4e89c821e
* verify(aggSig2, mergeInfos(sig1.aggInfo, sig2.aggInfo))
    * true
* verify(sig1, AggregationInfo(pk2, [7,8,9]))
    * false
* sig3 = sign([1,2,3], sk1)
* sig4 = sign([1,2,3,4], sk1)
* sig5 = sign([1,2], sk2)
* aggregate([sig3, sig4, sig5])
    * aggSig2: 0x8b11daf73cd05f2fe27809b74a7b4c65b1bb79cc1066bdf839d96b97e073c1a635d2ec048e0801b4a208118fdbbb63a516bab8755cc8d850862eeaa099540cd83621ff9db97b4ada857ef54c50715486217bd2ecb4517e05ab49380c041e159b
* verify(aggSig2, mergeInfos(sig3.aggInfo, sig4.aggInfo, sig5.aggInfo))
    * true
* sig1 = sk1.sign([1,2,3,40])
* sig2 = sk2.sign([5,6,70,201])
* sig3 = sk2.sign([1,2,3,40])
* sig4 = sk1.sign([9,10,11,12,13])
* sig5 = sk1.sign([1,2,3,40])
* sig6 = sk1.sign([15,63,244,92,0,1])
* sigL = aggregate([sig1, sig2])
* sigR = aggregate([sig3, sig4, sig5])
* verify(sigL)
    * true
* verify(sigR)
    * true
* aggregate([sigL, sigR, sig6])
    * sigFinal: 0x07969958fbf82e65bd13ba0749990764cac81cf10d923af9fdd2723f1e3910c3fdb874a67f9d511bb7e4920f8c01232b12e2fb5e64a7c2d177a475dab5c3729ca1f580301ccdef809c57a8846890265d195b694fa414a2a3aa55c32837fddd80
* verify(sigFinal)
    * true

### Signature division
* divide(sigFinal, [sig2, sig5, sig6])
    * quotient: 0x8ebc8a73a2291e689ce51769ff87e517be6089fd0627b2ce3cd2f0ee1ce134b39c4da40928954175014e9bbe623d845d0bdba8bfd2a85af9507ddf145579480132b676f027381314d983a63842fcc7bf5c8c088461e3ebb04dcf86b431d6238f
* verify(quotient)
    * true
* divide(quotient, [sig6])
    * throws due to not subset
* divide(sigFinal, [sig1])
    * does not throw
* divide(sig_final, [sigL])
    * throws due to not unique
* sig7 = sign([9,10,11,12,13], sk2)
* sig8 = sign([15,63,244,92,0,1], sk2)
* sigFinal2 = aggregate([sigFinal, aggregate([sig7, sig8])])
* divide(sigFinal2, aggregate([sig7, sig8]))
    * quotient2: 0x06af6930bd06838f2e4b00b62911fb290245cce503ccf5bfc2901459897731dd08fc4c56dbde75a11677ccfbfa61ab8b14735fddc66a02b7aeebb54ab9a41488f89f641d83d4515c4dd20dfcf28cbbccb1472c327f0780be3a90c005c58a47d3
* verify(quotient2)
    * true

### HD keys
* esk = ExtendedPrivateKey([1, 50, 6, 244, 24, 199, 1, 25])
* esk.publicKeyFigerprint
    * 0xa4700b27
* esk.chainCode
    * 0xd8b12555b4cc5578951e4a7c80031e22019cc0dce168b3ed88115311b8feb1e3
* esk77 = esk.privateChild(77 + 2^31)
* esk77.publicKeyFingerprint
    * 0xa8063dcf
* esk77.chainCode
    * 0xf2c8e4269bb3e54f8179a5c6976d92ca14c3260dd729981e9d15f53049fd698b
* esk.privateChild(3).privateChild(17).publicKeyFingerprint
    * 0xff26a31f
* esk.extendedPublicKey.publicChild(3).publicChild(17).publicKeyFingerprint
    * 0xff26a31f