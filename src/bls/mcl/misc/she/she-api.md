# she ; Two-level homomorphic encryption library for browser/Node.js by WebAssembly

# Abstract
she is a somewhat(two-level) homomorphic encryption library,
which is based on pairings.
This library supports polynomially many homomorphic additions and
one multiplication over encrypted data.

Especially, the inner products of two encrypted integer vectors such as Enc(x) = (Enc(x_i)), Enc(y) = (Enc(y_i))
can be computed.

Sum_i Enc(x_i) Enc(y_i) = Enc(Sum_i x_i y_i).

# Features
* supports the latest pairing based algorithm
    * [Efficient Two-level Homomorphic Encryption in Prime-order Bilinear Groups and A Fast Implementation in WebAssembly : ASIA CCS2018](http://asiaccs2018.org/?page_id=632)
* supports Windows(x64), Linux(x64, ARM64), OSX(x64)
* supports JavaScript(WebAssembly), Chrome, Firefox, Safari(contains Android, iPhone), Node.js

# Classes

## Main classes
* secret key class ; SecretKey
* public key class ; PublicKey
* ciphertext class ; CipherTextG1, CipherTextG2, CipherTextGT
* zero-knowledge proof class ; ZkpBin, ZkpEq, ZkpBinEq

## Encryption and decryption
* create the corresponding public key from a secret key
* encrypt an integer(plaintext) with a public key
* decrypt a ciphertext with a secret key

## Homomorphic operations
* homomorphic addition/subtraction over ciphertexts of the same ciphertext class
* homomprphic multiplication over ciphertext of CipherTextG1 and CipherTextG2
    * The class of the result is CipherTextGT.

## Important notation of decryption
* This library requires to solve a small DLP to decrypt a ciphertext.
* The decryption timing is O(m/s), where s is the size of table to solve DLP, and m is the size fo a plaintext.
* call `setRangeForDLP(s)` to set the table size.
    * The maximum `m/s` is set by `setTryNum(tryNum)`.

## Zero-knowledge proof class
* A zero-knowledge proof is simultaneously created when encrypting a plaintext `m`.
* The restriction according to `m` can be verified with a created zero-knowledge proof and a public key.

# Setup for JavaScript(JS)

## for Node.js

```
>npm install she-wasm
>node
>const she = require('she-wasm')
```

## for a browser

Copy `she.js`, `she\_c.js`, `she\_c.wasm` to your directory from [she-wasm](https://github.com/herumi/she-wasm/),
and read `she.js`.
```
// HTML
<script src="she.js"></script>
```

## A sample for JS

```js
// initialize a library
she.init().then(() => {
  const sec = new she.SecretKey()
  // initialize a secret key by CSPRNG(cryptographically secure pseudo random number generator)
  sec.setByCSPRNG()

  // create a public key from a secret key
  const pub = sec.getPublicKey()

  const m1 = 1
  const m2 = 2
  const m3 = 3
  const m4 = -1

  // encrypt m1 and m2 as CipherTextG1 class
  const c11 = pub.encG1(m1)
  const c12 = pub.encG1(m2)

  // encrypt m3 and m4 as CipherTextG2 class
  const c21 = pub.encG2(m3)
  const c22 = pub.encG2(m4)

  // add c11 and c12, c21 and c22 respectively
  const c1 = she.add(c11, c12)
  const c2 = she.add(c21, c22)

  // get ct as a CipherTextGT class by multiplying c1 with c2
  const ct = she.mul(c1, c2)

  // decrypt ct
  console.log(`(${m1} + ${m2}) * (${m3} + ${m4}) = ${sec.dec(ct)}`)
})
```

# A sample for C++
How to build the library, see [mcl](https://github.com/herumi/mcl/#installation-requirements).
```c++
#include <mcl/she.hpp>
int main()
    try
{
    using namespace mcl::she;
    // initialize a library
    init();

    SecretKey sec;

    // initialize a secret key by CSPRNG
    sec.setByCSPRNG();

    // create a public key from a secret key
    PublicKey pub;
    sec.getPublicKey(pub);

    int m1 = 1;
    int m2 = 2;
    int m3 = 3;
    int m4 = -1;

    // encrypt m1 and m2 as CipherTextG1 class
    CipherTextG1 c11, c12;
    pub.enc(c11, m1);
    pub.enc(c12, m2);

    // encrypt m3 and m4 as CipherTextG2 class
    CipherTextG2 c21, c22;
    pub.enc(c21, m3);
    pub.enc(c22, m4);

    // add c11 and c12, c21 and c22 respectively
    CipherTextG1 c1;
    CipherTextG2 c2;
    CipherTextG1::add(c1, c11, c12);
    CipherTextG2::add(c2, c21, c22);

    // get ct as a CipherTextGT class by multiplying c1 with c2
    CipherTextGT ct;
    CipherTextGT::mul(ct, c1, c2);

    // decrypt ct
    printf("(%d + %d) * (%d + %d) = %d\n", m1, m2, m3, m4, (int)sec.dec(ct));
} catch (std::exception& e) {
    printf("ERR %s\n", e.what());
    return 1;
}

```
# Class method

## Serialization(C++)

* `setStr(const std::string& str, int ioMode = 0)`
    * set a value by `str` according to `ioMode`

* `getStr(std::string& str, int ioMode = 0) const`
* `std::string getStr(int ioMode = 0) const`
    * get a string `str` according to `ioMode`
* `size_t serialize(void *buf, size_t maxBufSize) const`
    * serialize a value to buf which has maxBufSize byte size
    * return the byte size to be written in `buf`
    * return zero if error
* `size_t deserialize(const void *buf, size_t bufSize)`
    * deserialize a value from buf which has bufSize byte size
    * return the byte size to be read from `buf`
    * return zero if error

## Serialization(JS)

* `deserialize(s)`
    * deserialize from `s` as Uint8Array type
* `serialize()`
    * serialize a value and return Uint8Array value
* `deserializeHexStr(s)`
    * deserialize as a hexadecimal string
* `serializeToHexStr()`
    * serialize as a hexadecimal string

## ioMode

* 2 ; binary number
* 10 ; decimal number
* 16 ; hexadecimal number
* IoPrefix ; append a prefix 0b(resp. 2) or 0x(resp. 16)
* IoEcAffine ; affine coordinate (for only G1, G2)
* IoEcProj ; projective coordinate (for only G1, G2)
* IoSerialize ; same as serialize()/deserialize()

## Notation
* the namespace of C++ is `mcl::she`
* CT means one of CipherTextG1, CipherTextG2, CipherTextGT
* The range of plaintext is rectricted as a 32-bit integer for JS

## SecretKey class

* `void setByCSPRNG()`(C++)
* `void setByCSPRNG()`(JS)
    * set a secret key by CSPRNG(cryptographically secure pseudo random number generator)

* `int64_t dec(const CT& c) const`(C++)
* `int dec(CT c)`(JS)
    * decrypt `c`
* `int64_t decViaGT(const CipherTextG1& c) const`(C++)
* `int64_t decViaGT(const CipherTextG2& c) const`(C++)
* `int decViaGT(CT c)`(JS)
    * decrypt `c` through CipherTextGT
* `bool isZero(const CT& c) const`(C++)
* `bool isZero(CT c)`(JS)
    * return true if decryption of `c` is zero
    * it is faster than the timing of comparision with zero after decrypting `c`

## PublicKey, PrecomputedPublicKey class
`PrecomputedPublicKey` is a faster version of `PublicKey`

* `void PrecomputedPublicKey::init(const PublicKey& pub)`(C++)
* `void PrecomputedPublicKey::init(pub)`(JS)
    * initialize `PrecomputedPublicKey` by a public key `pub`

* `PrecomputedPublicKey::destroy()`(JS)
    * It is necessary to call this method if this instance becomes unnecessary
    * otherwise a memory leak will be caused

PK means PublicKey or PrecomputedPublicKey

* `void PK::enc(CT& c, int64_t m) const`(C++)
* `CipherTextG1 PK::encG1(m)`(JS)
* `CipherTextG2 PK::encG2(m)`(JS)
* `CipherTextGT PK::encGT(m)`(JS)
    * encrypt `m` and set `c`(or return the value)

* `void PK::reRand(CT& c) const`(C++)
* `CT PK::reRand(CT c)`(JS)
    * rerandomize `c`
    * For `c = Enc(m)`, the rerandomized ciphertext is hard to detect if it is generated by the rerandomization
    or an encrypted `m` freshly again.

* `void convert(CipherTextGT& cm, const CT& ca) const`
* `CipherTextGT convert(CT ca)`
   * convert `ca`(CipherTextG1 or CipherTextG2) to `CipherTextGT` class

## CipherText class

* `void CT::add(CT& z, const CT& x const CT& y)`(C++)
* `CT she.add(CT x, CT y)`(JS)
    * add `x` and `y` and set the value to `z`(or return the value)
* `void CT::sub(CT& z, const CT& x const CT& y)`(C++)
* `CT she.sub(CT x, CT y)`(JS)
    * subtract `x` and `y` and set the value to `z`(or return the value)
* `void CT::neg(CT& y, const CT& x)`(C++)
* `CT she.neg(CT x)`(JS)
    * negate `x` and set the value to `y`(or return the value)
* `void CT::mul(CT& z, const CT& x, int y)`(C++)
* `CT she.mulInt(CT x, int y)`(JS)
    * multiple `x` and `y` and set the value `y`(or return the value)

* `void CipherTextGT::mul(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)`(C++)
* `CipherTextGT she.mul(CipherTextG1 x, CipherTextG2 y)`(JS)
    * multiple `x` and `y` and set the value `y`(or return the value)

* `void CipherTextGT::mulML(CipherTextGT& z, const CipherTextG1& x, const CipherTextG2& y)`(C++)
    * multiple(only Miller Loop) `x` and `y` and set the value `y`(or return the value)

* `CipherTextGT::finalExp(CipherText& , const CipherTextG1& x, const CipherTextG2& y)`(C++)
    * mul(a, b) = finalExp(mulML(a, b))
    * add(mul(a, b), mul(c, d)) = finalExp(add(mulML(a, b), mulML(c, d)))
    * i.e., innor product can be computed as once calling `finalExp` after computing `mulML` for each elements of two vectors and adding all

## Zero knowledge proof class

### Abstract
* ZkpBin ; verify whether `m = 0` or `1` for ciphertexts `encGi(m)(i = 1, 2, T)`
* ZkpEq ; verify whether `m1 = m2` for ciphertexts `encG1(m1)` and `encG2(m2)`
* ZkpBinEq ; verify whether `m1 = m2 = 0` or `1` for ciphertexts `encG1(m1)` and `encG2(m2)`

### API
- SK = SecretKey
- PK = PublicKey or PrecomputedPublicKey
- AUX = AuxiliaryForZkpDecGT

* `void PK::encWithZkpBin(CipherTextG1& c, Zkp& zkp, int m) const`(C++)
* `void PK::encWithZkpBin(CipherTextG2& c, Zkp& zkp, int m) const`(C++)
* `[CipherTextG1, ZkpBin] PK::encWithZkpBinG1(m)`(JS)
* `[CipherTextG2, ZkpBin] PK::encWithZkpBinG2(m)`(JS)
    * encrypt `m`(=0 or 1) and set the ciphertext `c` and zero-knowledge proof `zkp`(or returns [c, zkp])
    * throw exception if m != 0 and m != 1
* `void PK::encWithZkpEq(CipherTextG1& c1, CipherTextG2& c2, ZkpEq& zkp, const INT& m) const`(C++)
* `[CipherTextG1, CipherTextG2, ZkpEq] PK::encWithZkpEq(m)`(JS)
    * encrypt `m` and set the ciphertext `c1`, `c2` and zero-knowledge proof `zk`(or returns [c1, c2, zkp])
* `void PK::encWithZkpBinEq(CipherTextG1& c1, CipherTextG2& c2, ZkpBinEq& zkp, int m) const`(C++)
* `[CipherTextG1, CipherTextG2, ZkpEqBin] PK::encWithZkpBinEq(m)`(JS)
    * encrypt `m`(=0 or 1) and set ciphertexts `c1`, `c2` and zero-knowledge proof `zkp`(or returns [c1, c2, zkp])
    * throw exception if m != 0 and m != 1
* `SK::decWithZkp(DecZkpDec& zkp, const CipherTextG1& c, const PublicKey& pub) const`(C++)
* `[m, ZkpDecG1] SK::decWithZkpDec(c, pub)`(JS)
  * decrypt CipherTextG1 `c` and get `m` and zkp, which proves that `dec(c) = m`.
  * `pub` is used for reducing some computation.
* `SK::decWithZkpDec(ZkpDecGT& zkp, const CipherTextGT& c, const AuxiliaryForZkpDecGT& aux) const`(C++)
* `[m, ZkpDecGT] SK::decWithZkpDecGT(c, aux)`(JS)
  * decrypt CipherTextGT `c` and get `m` and zkp, which proves that `dec(c) = m`.
  * `aux = pub.getAuxiliaryForZkpDecGT()`, which is used for reducing some computation.

## Global functions

* `void init(const CurveParam& cp, size_t hashSize = 1024, size_t tryNum = 1)`(C++)
* `void init(curveType = she.BN254, hashSize = 1024, tryNum = 1)`(JS)
    * initialize a hashSize * 8-bytes table to solve a DLP with `hashSize` size and set maximum trying count `tryNum`.
    * the range `m` to be solvable is |m| <= hashSize * tryNum
* `void initG1only(int curveType, size_t hashSize = 1024, size_t tryNum = 1)`(C++)
    * init only G1 (for Lifted ElGamal Encryption with SECP256K1)
* `getHashTableGT().load(InputStream& is)`(C++)
* `she.loadTableForGTDLP(Uint8Array a)`(JS)
    * load a DLP table for CipherTextGT
    * reset the value of `hashSize` used in `init()`
    * `https://herumi.github.io/she-dlp-table/she-dlp-0-20-gt.bin` is a precomputed table
* `void useDecG1ViaGT(bool use)`(C++/JS)
* `void useDecG2ViaGT(bool use)`(C++/JS)
    * decrypt a ciphertext of CipherTextG1 and CipherTextG2 through CipherTextGT
    * it is better when decrypt a big value

# License

[modified new BSD License](https://github.com/herumi/mcl/blob/master/COPYRIGHT)

# Author

光成滋生 MITSUNARI Shigeo(herumi@nifty.com)
