## bls-signatures

Go library that implements BLS signatures with aggregation as
in [Boneh, Drijvers, Neven 2018](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html), using the relic
toolkit for cryptographic primitives (pairings, EC, hashing).

This library is a Go port of the [Chia Network's BLS lib](https://github.com/Chia-Network/bls-signatures).

### Usage

```bash
go get github.com/dashpay/bls-signatures/go-bindings
```

### Creating keys and signatures

```go
// seed data must not be less 32 bytes length
seed := []byte{
    0, 50, 6, 244, 24, 199, 1, 25,
    52, 88, 192, 19, 18, 12, 89, 6,
    220, 18, 102, 58, 209, 82, 12, 62,
    89, 110, 182, 9, 44, 20, 254, 22,
}

// create a scheme, available three schemes: BasicSchemeMPL, AugSchemeMPL and PopSchemeMPL 
scheme := NewAugSchemeMPL()

// generate a private key using a seed data
sk, err := scheme.KeyGen(seed)
if err != nil {
    panic(err.Error())
}
// get a public key
pk, err := sk.G1Element()
if err != nil {
    panic(err.Error())
}

msg := []byte{1, 2, 3, 4, 5}

// make a signature for a message
sig := scheme.Sign(sk, msg)

// verify the message signature 
if !scheme.Verify(pk, msg, sig) {
    panic("failed the signature verification")
}
```

### Serializing keys and signatures to bytes

```go  
skBytes := sk.Serialize()
pkBytes := pk.Serialize()
sigBytes := sig.Serialize()  
```

### Loading keys and signatures from bytes

```go
sk1, _ := PrivateKeyFromBytes(skBytes, false)
pk1, _ := G1ElementFromBytes(pkBytes)
sig1, _ := G2ElementFromBytes(sigBytes)
```

### Create aggregate signatures

```go
// Generate some more private keys
seed[0] = 1
sk1, _ := scheme.KeyGen(seed)
seed[0] = 2
sk2, _ := scheme.KeyGen(seed)
msg2 := []byte{1, 2, 3, 4, 5,6, 7}

// Generate first sig
pk1, _ := sk1.G1Element()
sig1 := scheme.Sign(sk1, msg)

// Generate second sig
pk2, _ := sk2.G1Element()
sig2 := scheme.Sign(sk2, msg2)

// Signatures can be non-interactively combined by anyone
var aggSig = scheme.AggregateSigs(sig1, sig2)

ok := scheme.AggregateVerify([]*G1Element{pk1, pk2}, [][]byte{msg, msg2}, aggSig)
if !ok {
    panic("failed a verification of the aggregated signature ")
}
```

### Arbitrary trees of aggregates

```go
seed[0] = 3
sk3, _ := scheme.KeyGen(seed)
pk3, _ := sk3.G1Element()
msg3 := []byte{100, 2, 254, 88, 90, 45, 23}
sig3 := scheme.Sign(sk3, msg3)

aggSigFinal := scheme.AggregateSigs(aggSig, sig3)
ok = scheme.AggregateVerify([]*G1Element{pk1, pk2, pk3}, [][]byte{msg, msg2, msg3}, aggSigFinal)
if !ok {
    panic("failed a verification of the aggregated signature ")
}
```

### Very fast verification with Proof of Possession scheme

```go
// create a proof possession scheme
popScheme := NewPopSchemeMPL()

// If the same msg is signed, you can use Proof of Possession (PopScheme) for efficiency
// A proof of possession MUST be passed around with the PK to ensure security.
popSig1 := popScheme.Sign(sk1, msg)
popSig2 := popScheme.Sign(sk2, msg)
popSig3 := popScheme.Sign(sk3, msg)
pop1 := popScheme.PopProve(sk1)
pop2 := popScheme.PopProve(sk2)
pop3 := popScheme.PopProve(sk3)

ok = popScheme.PopVerify(pk1, pop1)
if !ok {
    panic("failed a verification")
}
ok = popScheme.PopVerify(pk2, pop2)
if !ok {
    panic("failed a verification")
}
ok = popScheme.PopVerify(pk3, pop3)
if !ok {
    panic("failed a verification")
}

popSigAgg := popScheme.AggregateSigs(popSig1, popSig2, popSig3)
ok = popScheme.FastAggregateVerify([]*G1Element{pk1, pk2, pk3}, msg, popSigAgg)
if !ok {
    panic("failed a verification")
}

// Aggregate public key, indistinguishable from a single public key
var popAggPk = pk1.Add(pk2).Add(pk3)
ok = popScheme.Verify(popAggPk, msg, popSigAgg)
if !ok {
    panic("failed a verification")
}

// Aggregate private keys
var aggSk = PrivateKeyAggregate(sk1, sk2, sk3)
ok = popScheme.Sign(aggSk, msg).EqualTo(popSigAgg)
if !ok {
    panic("failed a verification")
}
```

### HD keys using [EIP-2333](https://github.com/ethereum/EIPs/pull/2333)

```go
// You can derive 'child' keys from any key, to create arbitrary trees. 4 byte indeces are used.
// Hardened (more secure, but no parent pk -> child pk)
masterSk, _ := augScheme.KeyGen(seed)

// Unhardened (less secure, but can go from parent pk -> child pk), BIP32 style
masterPk, _ := masterSk.G1Element()
childU := augScheme.DeriveChildSkUnhardened(masterSk, 22)
grandchildU := augScheme.DeriveChildSkUnhardened(childU, 0)

childUPk := augScheme.DeriveChildPkUnhardened(masterPk, 22)
grandchildUPk := augScheme.DeriveChildPkUnhardened(childUPk, 0)

pkChildU, _ := grandchildU.G1Element()
ok = grandchildUPk.EqualTo(pkChildU)
if !ok {
    panic("keys are not equal")
}
```

Do not forget to handle the errors properly, this part of the code was omitted deliberately

Use cases can be found in the [original lib's readme](../README.md).

__Important note:__ Since this library is a port of the c++ library, so every piece of memory allocated by the library
MUST be released on our own in GO. To release the memory is used `runtime.SetFinalizer` function, that will invoke
automatically before GC release a memory allocated by GO.

### Run tests

To run tests, build the library, then go to the `go-bindings` folder in the build directory and run

```bash
make test
```
