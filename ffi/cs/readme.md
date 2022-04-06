# C# binding of BLS threshold signature library

# Installation Requirements

* Visual Studio 2017 or later
* C# 7.2 or later
* .NET Framework 4.5.2 or later

# How to build

```
md work
cd work
git clone https://github.com/herumi/cybozulib_ext
git clone https://github.com/herumi/mcl
git clone https://github.com/herumi/bls
cd bls
mklib dll
```
bls/bin/*.dll are created

# How to build a sample

Open bls/ffi/cs/bls.sln and exec it.

# class and API

## API

* `Init(int curveType = BN254);`
    * initialize this library with a curve `curveType`.
    * curveType = BN254 or BLS12_381
* `SecretKey ShareSecretKey(in SecretKey[] msk, in Id id);`
    * generate the shared secret key from a sequence of master secret keys msk and Id.
* `SecretKey RecoverSecretKey(in SecretKey[] secVec, in Id[] idVec);`
    * recover the secret key from a sequence of secret keys secVec and idVec.
* `PublicKey SharePublicKey(in PublicKey[] mpk, in Id id);`
    * generate the shared public key from a sequence of master public keys mpk and Id.
* `PublicKey RecoverPublicKey(in PublicKey[] pubVec, in Id[] idVec);`
    * recover the public key from a sequence of public keys pubVec and idVec.
* `Signature RecoverSign(in Signature[] sigVec, in Id[] idVec);`
    * recover the signature from a sequence of signatures siVec and idVec.

## Id

Identifier class

* `byte[] Serialize();`
    * serialize Id
* `void Deserialize(byte[] buf);`
    * deserialize from byte[] buf
* `bool IsEqual(in Id rhs);`
    * equality
* `void SetDecStr(string s);`
    * set by a decimal string s
* `void SetHexStr(string s);`
    * set by a hexadecimal string s
* `void SetInt(int x);`
    * set an integer x
* `string GetDecStr();`
    * get a decimal string
* `string GetHexStr();`
    * get a hexadecimal string

## SecretKey

* `byte[] Serialize();`
    * serialize SecretKey
* `void Deserialize(byte[] buf);`
    * deserialize from byte[] buf
* `bool IsEqual(in SecretKey rhs);`
    * equality
* `string GetDecStr();`
    * get a decimal string
* `string GetHexStr();`
    * get a hexadecimal string
* `void Add(in SecretKey rhs);`
    * add a secret key rhs
* `void SetByCSPRNG();`
    * set a secret key by cryptographically secure pseudo random number generator
* `void SetHashOf(string s);`
    * set a secret key by a hash of string s
* `PublicKey GetPublicKey();`
    * get the corresponding public key to a secret key
* `Signature Sign(string m);`
    * sign a string m
* `Signature GetPop();`
    * get a PoP (Proof Of Posession) for a secret key

## PublicKey

* `byte[] Serialize();`
    * serialize PublicKey
* `void Deserialize(byte[] buf);`
    * deserialize from byte[] buf
* `bool IsEqual(in PublicKey rhs);`
    * equality
* `void Add(in PublicKey rhs);`
    * add a public key rhs
* `string GetDecStr();`
    * get a decimal string
* `string GetHexStr();`
    * get a hexadecimal string
* `bool Verify(in Signature sig, string m);`
    * verify the validness of the sig with m
* `bool VerifyPop(in Signature pop);`
    * verify the validness of PoP

## Signature

* `byte[] Serialize();`
    * serialize Signature
* `void Deserialize(byte[] buf);`
    * deserialize from byte[] buf
* `bool IsEqual(in Signature rhs);`
    * equality
* `void Add(in Signature rhs);`
    * add a signature key rhs
* `string GetDecStr();`
    * get a decimal string
* `string GetHexStr();`
    * get a hexadecimal string

## How to use

### A minimum sample

```
using static BLS;

Init(BN254); // init library
SecretKey sec;
sec.SetByCSPRNG(); // init secret key
PublicKey pub = sec.GetPublicKey(); // get public key
string m = "abc";
Signature sig = sec.Sign(m); // create signature
if (pub.Verify(sig, m))) {
  // signature is verified
}
```

### Aggregate signature
```
Init(BN254); // init library
const int n = 10;
const string m = "abc";
SecretKey[] secVec = new SecretKey[n];
PublicKey[] pubVec = new PublicKey[n];
Signature[] popVec = new Signature[n];
Signature[] sigVec = new Signature[n];

for (int i = 0; i < n; i++) {
  secVec[i].SetByCSPRNG(); // init secret key
  pubVec[i] = secVec[i].GetPublicKey(); // get public key
  popVec[i] = secVec[i].GetPop(); // get a proof of Possesion (PoP)
  sigVec[i] = secVec[i].Sign(m); // create signature
}

SecretKey secAgg;
PublicKey pubAgg;
Signature sigAgg;
for (int i = 0; i < n; i++) {
  // verify PoP
  if (pubVec[i].VerifyPop(popVec[i]))) {
    // error
    return;
  }
  pubAgg.Add(pubVec[i]); // aggregate public key
  sigAgg.Add(sigVec[i]); // aggregate signature
}
if (pubAgg.Verify(sigAgg, m)) {
  // aggregated signature is verified
}
```

# License

modified new BSD License
http://opensource.org/licenses/BSD-3-Clause

# Author

(C)2019 MITSUNARI Shigeo(herumi@nifty.com) All rights reserved.
