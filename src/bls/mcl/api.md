# C/C++ API

## Minimum sample

A sample of how to use BLS12-381:

### C
[sample/pairing_c.c](sample/pairing_c.c)

```
cd mcl
make -j4
make bin/pairing_c.exe && bin/pairing_c.exe
```

### C++
[sample/pairing.cpp](sample/pairing.cpp)

```
cd mcl
make -j4
make bin/pairing.exe && bin/pairing.exe
```

## Header and libraries

### C
To use BLS12-381, include `<mcl/bn_c384_256.h>` and link
- libmclbn384_256.{a,so}
- libmcl.{a,so} ; core library

`384_256` means the max bit size of `Fp` is 384, and that size of `Fr` is 256.

### C++
include `<mcl/bls12_381.hpp>` and link
- libmcl.{a,so} ; core library

## Notation

The elliptic equation of a curve E is `E: y^2 = x^3 + b`.

- `Fp` ; a finite field of a prime order `p`, where a curve is defined over.
- `Fr` ; a finite field of a prime order `r`.
- `Fp2` ; the field extension over Fp with degree 2. Fp[i] / (i^2 + 1).
- `Fp6` ; the field extension over Fp2 with degree 3. Fp2[v] / (v^3 - Xi) where Xi = i + 1.
- `Fp12` ; the field extension over Fp6 with degree 2. Fp6[w] / (w^2 - v).
- `G1` ; the cyclic subgroup of E(Fp).
- `G2` ; the cyclic subgroup of the inverse image of E'(Fp^2) under a twisting isomorphism from E' to E.
- `GT` ; the cyclic subgroup of Fp12.
  - `G1`, `G2`, and `GT` have the order `r`.

The pairing e: G1 x G2 -> GT is the optimal ate pairing.

mcl treats `G1` and `G2` as an additive group and `GT` as a multiplicative group.

- `mclSize` ; `unsigned int` if WebAssembly else `size_t`

### Curve Parameter
r = |G1| = |G2| = |GT|

curveType   | b| r and p |
------------|--|------------------|
BN254       | 2|r = 0x2523648240000001ba344d8000000007ff9f800000000010a10000000000000d <br> p = 0x2523648240000001ba344d80000000086121000000000013a700000000000013 |
BLS12-381   | 4|r = 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001 <br> p = 0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab |
BN381       | 2|r = 0x240026400f3d82b2e42de125b00158405b710818ac000007e0042f008e3e00000000001080046200000000000000000d <br> p = 0x240026400f3d82b2e42de125b00158405b710818ac00000840046200950400000000001380052e000000000000000013 |

## C Structures

### `mclBnFp`
This is a struct of `Fp`. The value is stored as a Montgomery representation.

### `mclBnFr`
This is a struct of `Fr`. The value is stored as a Montgomery representation.

### `mclBnFp2`
This is a struct of `Fp2` with a member `mclBnFp d[2]`.

An element `x` of `Fp2` is represented as `x = d[0] + d[1] i` where `i^2 = -1`.

### `mclBnG1`
This is a struct of `G1` with three members `x`, `y`, `z` of type `mclBnFp`.

An element `P` of `G1` is represented as `P = [x:y:z]` of a Jacobi coordinate.

### `mclBnG2`
This is a struct of `G2` with three members `x`, `y`, `z` of type `mclBnFp2`.

An element `Q` of `G2` is represented as `Q = [x:y:z]` of a Jacobi coordinate.

### `mclBnGT`

This is a struct of `GT` with a member `mclBnFp d[12]`.

## C++ Structures

The namespace is `mcl::bn`.

### `Fp`
This is a class of `Fp`.

### `Fr`
This is a class of `Fr`.

### `Fp2`
This is a struct of `Fp2` with a member `a` and `b` of type `Fp`.

An element `x` of `Fp2` is represented as `x = a + b i` where `i^2 = -1`.

### `Fp6`
This is a struct of `Fp6` with a member `a`, `b`, and `c` of type `Fp2`.

An element `x` of `Fp6` is represented as `x = a + b v + c v^2` where `v^3 = xi := 1 + i`.

### `Fp12`
This is a struct of `Fp12` with a member `a` and `b` of type `Fp6`.

An element `x` of `Fp12` is represented as `x = a + b w` where `w^2 = v`.

### `G1`
This is a struct of `G1` with three members `x`, `y`, `z` of type `Fp`.

An element `P` of `G1` is represented as `P = [x:y:z]` of a Jacobi coordinate.

### `G2`
This is a struct of `G2` with three members `x`, `y`, `z` of type `Fp2`.

An element `Q` of `G2` is represented as `Q = [x:y:z]` of a Jacobi coordinate.

### `GT`

`GT` is an alias of `Fp12`.
But it means a set `{ x in Fp12 | x^r = 1}`.

### sizeof

library           |MCLBN_FR_UNIT_SIZE|MCLBN_FP_UNIT_SIZE|sizeof Fr|sizeof Fp|
------------------|------------------|------------------|---------|---------|
libmclbn256.a     |          4       |         4        |   32    |   32    |
libmclbn384_256.a |          4       |         6        |   32    |   48    |
libmclbn384.a     |          6       |         6        |   48    |   48    |

## Thread safety
All functions except for initialization and changing global settings are thread-safe.

## Initialization

### C

Initialize mcl library. Call this function at first before calling the other functions.

```
int mclBn_init(int curve, int compiledTimeVar);
```

- `curve` ; specify the curve type
  - MCL_BN254 ; BN254 (a little faster if including `mcl/bn_c256.h` and linking `libmclbn256.{a,so}`)
  - MCL_BN_SNARK1 ; the same parameter used in libsnark
  - MCL_BLS12_381 ; BLS12-381
  - MCL_BN381_1 ; BN381 (include `mcl/bn_c384.h` and link `libmclbn384.{a,so}`)
- `compiledTimeVar` ; set `MCLBN_COMPILED_TIME_VAR`, which macro is used to make sure that
the values are the same when the library is built and used.
- return 0 if success.
- This is not thread safe.

### C++

```
void initPairing(<curve type>);
```
curve type is defined in `mcl/curve_type.h`.
- BN254
- BN_SNARK1
- BN381_1
- BLS12_381

## Global setting

```
int mclBn_setMapToMode(int mode);
```
- `mode = MCL_MAP_TO_MODE_ORIGINAL` : the old hash-to-curve (for backward compatibility)
- `mode = MCL_MAP_TO_MODE_HASH_TO_CURVE` : the hash-to-curve defined in [Hashing to Elliptic Curves](https://datatracker.ietf.org/doc/draft-irtf-cfrg-hash-to-curve/)

### Control to verify that a point of the elliptic curve has the order `r`.

This function affects `setStr()` and `deserialize()` for G1/G2.

### C
```
void mclBn_verifyOrderG1(int doVerify);
void mclBn_verifyOrderG2(int doVerify);
```

### C++
```
verifyOrderG1(bool doVerify);
verifyOrderG2(bool doVerify);
```

- verify if `doVerify` is 1 or does not. The default parameter is 0 because the cost of verification is not small.
- Set `doVerify = 1` if considering subgroup attack is necessary.
- This is not thread-safe.

## Setter / Getter

### Clear
Set `x` is zero.
```
void mclBnFr_clear(mclBnFr *x);
void mclBnFp_clear(mclBnFp *x);
void mclBnFp2_clear(mclBnFp2 *x);
void mclBnG1_clear(mclBnG1 *x);
void mclBnG2_clear(mclBnG2 *x);
void mclBnGT_clear(mclBnGT *x);
```

C++
```
T::clear();
```

### Set `x` to `y`.
```
void mclBnFp_setInt(mclBnFp *y, mclInt x);
void mclBnFr_setInt(mclBnFr *y, mclInt x);
void mclBnGT_setInt(mclBnGT *y, mclInt x);
```

C++
```
T x = <integer literal>;
```

### Set `buf[0..bufSize-1]` to `x` with masking according to the following way.
```
int mclBnFp_setLittleEndian(mclBnFp *x, const void *buf, mclSize bufSize);
int mclBnFr_setLittleEndian(mclBnFr *x, const void *buf, mclSize bufSize);
```

C++
```
T::setArrayMask(const uint8_t *buf, size_t n);
```

1. set x = buf[0..bufSize-1] as little endian
2. x &= (1 << bitLen(r)) - 1
3. if (x >= r) x &= (1 << (bitLen(r) - 1)) - 1

- always return 0

### Set (`buf[0..bufSize-1]` mod `p` or `r`) to `x`.
```
int mclBnFp_setLittleEndianMod(mclBnFp *x, const void *buf, mclSize bufSize);
int mclBnFr_setLittleEndianMod(mclBnFr *x, const void *buf, mclSize bufSize);
```

C++
```
T::setLittleEndianMod(const uint8_t *buf, mclSize bufSize);
```

- return 0 if bufSize <= (sizeof(*x) * 8 * 2) else -1

### Get little-endian byte sequence `buf` corresponding to `x`
```
mclSize mclBnFr_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFr *x);
mclSize mclBnFp_getLittleEndian(void *buf, mclSize maxBufSize, const mclBnFp *x);
```

C++
```
size_t T::getLittleEndian(uint8_t *buf, size_t maxBufSize) const
```

- write `x` to `buf` as little endian
- return the written size if sucess else 0
- NOTE: `buf[0] = 0` and return 1 if `x` is zero.

### Serialization
### Serialize
```
mclSize mclBnFr_serialize(void *buf, mclSize maxBufSize, const mclBnFr *x);
mclSize mclBnG1_serialize(void *buf, mclSize maxBufSize, const mclBnG1 *x);
mclSize mclBnG2_serialize(void *buf, mclSize maxBufSize, const mclBnG2 *x);
mclSize mclBnGT_serialize(void *buf, mclSize maxBufSize, const mclBnGT *x);
mclSize mclBnFp_serialize(void *buf, mclSize maxBufSize, const mclBnFp *x);
mclSize mclBnFp2_serialize(void *buf, mclSize maxBufSize, const mclBnFp2 *x);
```

C++
```
mclSize T::serialize(void *buf, mclSize maxBufSize) const;
```

- serialize `x` into `buf[0..maxBufSize-1]`
- return written byte size if success else 0

### Serialization format
- `Fp`(resp.  `Fr`) ; a little endian byte sequence with a fixed size
  - the size is the return value of `mclBn_getFpByteSize()` (resp. `mclBn_getFpByteSize()`).
- `G1` ; a compressed fixed size
  - the size is equal to `mclBn_getG1ByteSize()` (=`mclBn_getFpByteSize()`).
- `G2` ; a compressed fixed size
  - the size is equal to `mclBn_getG1ByteSize() * 2`.

A pseudo-code to serialize `P` of `G1` (resp. `G2`):
```
size = mclBn_getG1ByteSize() # resp. mclBn_getG1ByteSize() * 2
if P is zero:
  return [0] * size
else:
  P = P.normalize()
  s = P.x.serialize()
  # x in Fp2 is odd <=> x.a is odd
  if P.y is odd: # resp. P.y.d[0] is odd
    s[byte-length(s) - 1] |= 0x80
  return s
```

### Ethereum serialization mode for BLS12-381
```
void mclBn_setETHserialization(int ETHserialization);
```
- Set `ETHserialization = 1` for Ethereum compatibility (default 0).
- See [FAQ](api.md#faq).

### Deserialize
```
mclSize mclBnFr_deserialize(mclBnFr *x, const void *buf, mclSize bufSize);
mclSize mclBnG1_deserialize(mclBnG1 *x, const void *buf, mclSize bufSize);
mclSize mclBnG2_deserialize(mclBnG2 *x, const void *buf, mclSize bufSize);
mclSize mclBnGT_deserialize(mclBnGT *x, const void *buf, mclSize bufSize);
mclSize mclBnFp_deserialize(mclBnFp *x, const void *buf, mclSize bufSize);
mclSize mclBnFp2_deserialize(mclBnFp2 *x, const void *buf, mclSize bufSize);
```

C++
```
mclSize T::deserialize(const void *buf, mclSize bufSize);
```

- deserialize `x` from `buf[0..bufSize-1]`
- return read size if success else 0

## String conversion
### Get string
```
mclSize mclBnFr_getStr(char *buf, mclSize maxBufSize, const mclBnFr *x, int ioMode);
mclSize mclBnG1_getStr(char *buf, mclSize maxBufSize, const mclBnG1 *x, int ioMode);
mclSize mclBnG2_getStr(char *buf, mclSize maxBufSize, const mclBnG2 *x, int ioMode);
mclSize mclBnGT_getStr(char *buf, mclSize maxBufSize, const mclBnGT *x, int ioMode);
mclSize mclBnFp_getStr(char *buf, mclSize maxBufSize, const mclBnFp *x, int ioMode);
```

C++
```
size_t T::getStr(char *buf, size_t maxBufSize, int iMode = 0) const
```

- write `x` to `buf` according to `ioMode`
- `ioMode`
  - 10 ; decimal number
  - 16 ; hexadecimal number
  - `MCLBN_IO_EC_PROJ` ; output as Jacobi coordinate
- return `strlen(buf)` if success else 0.

The meaning of the output of `G1`:
- `0` ; infinity
- `1 <x> <y>` ; affine coordinate
- `4 <x> <y> <z>` ; Jacobi coordinate
- the element `<x>` of `G2` outputs `d[0] d[1]`.

### Set string
```
int mclBnFr_setStr(mclBnFr *x, const char *buf, mclSize bufSize, int ioMode);
int mclBnG1_setStr(mclBnG1 *x, const char *buf, mclSize bufSize, int ioMode);
int mclBnG2_setStr(mclBnG2 *x, const char *buf, mclSize bufSize, int ioMode);
int mclBnGT_setStr(mclBnGT *x, const char *buf, mclSize bufSize, int ioMode);
int mclBnFp_setStr(mclBnFp *x, const char *buf, mclSize bufSize, int ioMode);
```

C++
```
void T::setStr(bool *pb, const char *str, int iMode = 0)
void T::setStr(const char *str, int iMode = 0)
```

- set `buf[0..bufSize-1]` to `x` accoring to `ioMode`
  - mask and truncate the value if it is greater than (r or p).
  - See [masking](api.md#set-buf0bufsize-1-to-x-with-masking-according-to-the-following-way)
- deny too large bufSize. The maximum length depends on compile options, but at least the bit length of the type of x.
- return 0 if success else -1
  - *pb = result of setStr or throw exception if error (C++)

If you want to use the same generators of BLS12-381 with [zkcrypto](https://github.com/zkcrypto/pairing/tree/master/src/bls12_381#g2) then,

```
mclBnG1 P;
mclBnG1_setStr(&P, "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569", 10);

mclBnG2 Q;
mclBnG2_setStr(&Q, "1 352701069587466618187139116011060144890029952792775240219908644239793785735715026873347600343865175952761926303160 3059144344244213709971259814753781636986470325476647558659373206291635324768958432433509563104347017837885763365758 1985150602287291935568054521177171638300868978215655730859378665066344726373823718423869104263333984641494340347905 927553665492332455747201965776037880757740193453592970025027978793976877002675564980949289727957565575433344219582");
```


## Set a random value
Set `x` by a cryptographically secure pseudo-random number generator.
```
int mclBnFr_setByCSPRNG(mclBnFr *x);
int mclBnFp_setByCSPRNG(mclBnFp *x);
```
C++
```
void T::setByCSPRNG()
```

### Change random generator function
```
void mclBn_setRandFunc(
  void *self,
  unsigned int (*readFunc)(void *self, void *buf, unsigned int bufSize)
);
```
- `self` ; user-defined pointer
- `readFunc` ; user-defined function, which writes random `bufSize` bytes to `buf` and returns `bufSize` if success else returns 0.
  - `readFunc` must be thread-safe.
- Set the default random function if `self == 0` and `readFunc == 0`.
- This is not thread-safe.

## Arithmetic operations
### neg / inv / sqr / add / sub / mul / div of `Fr`, `Fp`, `Fp2`, `GT`.
```
void mclBnFr_neg(mclBnFr *y, const mclBnFr *x);
void mclBnFr_inv(mclBnFr *y, const mclBnFr *x);
void mclBnFr_sqr(mclBnFr *y, const mclBnFr *x);
void mclBnFr_add(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
void mclBnFr_sub(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
void mclBnFr_mul(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);
void mclBnFr_div(mclBnFr *z, const mclBnFr *x, const mclBnFr *y);

void mclBnFp_neg(mclBnFp *y, const mclBnFp *x);
void mclBnFp_inv(mclBnFp *y, const mclBnFp *x);
void mclBnFp_sqr(mclBnFp *y, const mclBnFp *x);
void mclBnFp_add(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
void mclBnFp_sub(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
void mclBnFp_mul(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);
void mclBnFp_div(mclBnFp *z, const mclBnFp *x, const mclBnFp *y);

void mclBnFp2_neg(mclBnFp2 *y, const mclBnFp2 *x);
void mclBnFp2_inv(mclBnFp2 *y, const mclBnFp2 *x);
void mclBnFp2_sqr(mclBnFp2 *y, const mclBnFp2 *x);
void mclBnFp2_add(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
void mclBnFp2_sub(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
void mclBnFp2_mul(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);
void mclBnFp2_div(mclBnFp2 *z, const mclBnFp2 *x, const mclBnFp2 *y);

void mclBnGT_inv(mclBnGT *y, const mclBnGT *x); // y = a - bw for x = a + bw where Fp12 = Fp6[w]
void mclBnGT_sqr(mclBnGT *y, const mclBnGT *x);
void mclBnGT_mul(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
void mclBnGT_div(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
```
- use `mclBnGT_invGeneric` for an element in Fp12 - GT.

- NOTE: The following functions do NOT return a GT element because GT is a multiplicative group.

```
void mclBnGT_neg(mclBnGT *y, const mclBnGT *x);
void mclBnGT_add(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
void mclBnGT_sub(mclBnGT *z, const mclBnGT *x, const mclBnGT *y);
```

C++
- `+`, `-`, `*`, `/`
  - T::add(T& z, const T& x, const T& y);
  - T::sub(T& z, const T& x, const T& y);
  - T::mul(T& z, const T& x, const T& y);
  - T::div(T& z, const T& x, const T& y);
  - T::neg(T& y, const T& x);
  - T::inv(T& y, const T& x);

### Square root of `x`.
```
int mclBnFr_squareRoot(mclBnFr *y, const mclBnFr *x);
int mclBnFp_squareRoot(mclBnFp *y, const mclBnFp *x);
int mclBnFp2_squareRoot(mclBnFp2 *y, const mclBnFp2 *x);
```
- `y` is one of a square root of `x` if `y` exists.
- return 0 if success else -1

### add / sub / dbl / neg for `G1` and `G2`.
```
void mclBnG1_neg(mclBnG1 *y, const mclBnG1 *x);
void mclBnG1_dbl(mclBnG1 *y, const mclBnG1 *x);
void mclBnG1_add(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y);
void mclBnG1_sub(mclBnG1 *z, const mclBnG1 *x, const mclBnG1 *y);

void mclBnG2_neg(mclBnG2 *y, const mclBnG2 *x);
void mclBnG2_dbl(mclBnG2 *y, const mclBnG2 *x);
void mclBnG2_add(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y);
void mclBnG2_sub(mclBnG2 *z, const mclBnG2 *x, const mclBnG2 *y);
```
C++
- `+`, `-`
  - T::add(T& z, const T& x, const T& y);
  - T::sub(T& z, const T& x, const T& y);
  - T::neg(T& y, const T& x);

### Convert a point from Jacobi coordinate to affine.
```
void mclBnG1_normalize(mclBnG1 *y, const mclBnG1 *x);
void mclBnG2_normalize(mclBnG2 *y, const mclBnG2 *x);
```

C++
```
T::normalize(T& y, const T& x)
```

- convert `[x:y:z]` to `[x:y:1]` if `z != 0` else `[*:*:0]`

### scalar multiplication
```
void mclBnG1_mul(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y);
void mclBnG2_mul(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y);
void mclBnGT_pow(mclBnGT *z, const mclBnGT *x, const mclBnFr *y);
```
C++
```
T::mul(const T& z, const T& x, const Fr& y);
```

- z = x * y for G1 / G2
- z = pow(x, y) for GT

- use `mclBnGT_powGeneric` for an element in Fp12 - GT.

### multi-scalar multiplication
```
void mclBnG1_mulVec(mclBnG1 *z, const mclBnG1 *x, const mclBnFr *y, mclSize n);
void mclBnG2_mulVec(mclBnG2 *z, const mclBnG2 *x, const mclBnFr *y, mclSize n);
void mclBnGT_powVec(mclBnGT *z, const mclBnGT *x, const mclBnFr *y, mclSize n);
```
C++
```
T::mulVec(T, const T&, const Fr *y, size_t n);
```

- z = sum_{i=0}^{n-1} mul(x[i], y[i]) for G1 / G2.
- z = prod_{i=0}^{n-1} pow(x[i], y[i]) for GT.

## hash-to-curve function
### Set hash of `buf[0..bufSize-1]` to `x`
```
int mclBnFr_setHashOf(mclBnFr *x, const void *buf, mclSize bufSize);
int mclBnFp_setHashOf(mclBnFp *x, const void *buf, mclSize bufSize);
```
C++
```
T::setHashOf(const void *msg, size_t msgSize);
```

- always return 0
- use SHA-256 if sizeof(*x) <= 256 else SHA-512
- set according to the same way as `setLittleEndian`.

### map `x` to G1 / G2.
```
int mclBnFp_mapToG1(mclBnG1 *y, const mclBnFp *x);
int mclBnFp2_mapToG2(mclBnG2 *y, const mclBnFp2 *x);
```

C++
```
void mapToG1(G1& P, const Fp& x);
void mapToG2(G2& P, const Fp2& x);
```

- See `struct MapTo` in `mcl/bn.hpp` for the detail of the algorithm.
- return 0 if success else -1

### hash and map to G1 / G2.
```
int mclBnG1_hashAndMapTo(mclBnG1 *x, const void *buf, mclSize bufSize);
int mclBnG2_hashAndMapTo(mclBnG2 *x, const void *buf, mclSize bufSize);
```

C++
```
void hashAndMapToG1(G1& P, const void *buf, size_t bufSize);
void hashAndMapToG2(G2& P, const void *buf, size_t bufSize);
```
- Combine `setHashOf` and `mapTo` functions

## Pairing operations
The pairing function `e(P, Q)` is consist of two parts:
  - `MillerLoop(P, Q)`
  - `finalExp(x)`

`finalExp` satisfies the following properties:
  - `e(P, Q) = finalExp(MillerLoop(P, Q))`
  - `e(P1, Q1) e(P2, Q2) = finalExp(MillerLoop(P1, Q1) MillerLoop(P2, Q2))`

### pairing
```
void mclBn_pairing(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y);
```
C++
```
void pairing(GT& z, const G1& x, const G2& y);
```

### millerLoop
```
void mclBn_millerLoop(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y);
```
C++
```
void millerLoop(GT& z, const G1& x, const G2& y);
```
### finalExp
```
void mclBn_finalExp(mclBnGT *y, const mclBnGT *x);
```
C++
```
void finalExp(GT& y, const GT& x);
```

## Variants of MillerLoop
### multi pairing
```
void mclBn_millerLoopVec(mclBnGT *z, const mclBnG1 *x, const mclBnG2 *y, mclSize n);
```
C++
```
void millerLoopVec(GT& z, const G1 *x, const G2 *y, size_t n);
```
- This function is for multi-pairing
  - computes prod_{i=0}^{n-1} MillerLoop(x[i], y[i])
  - prod_{i=0}^{n-1} e(x[i], y[i]) = finalExp(prod_{i=0}^{n-1} MillerLoop(x[i], y[i]))

### pairing for a fixed point of G2
```
int mclBn_getUint64NumToPrecompute(void);
void mclBn_precomputeG2(uint64_t *Qbuf, const mclBnG2 *Q);
void mclBn_precomputedMillerLoop(mclBnGT *f, const mclBnG1 *P, const uint64_t *Qbuf);
```
These functions is the same computation of `pairing(P, Q);` as the followings:
```
uint64_t *Qbuf = (uint64_t*)malloc(mclBn_getUint64NumToPrecompute() * sizeof(uint64_t));
mclBn_precomputeG2(Qbuf, Q); // precomputing of Q
mclBn_precomputedMillerLoop(f, P, Qbuf); // pairing of any P of G1 and the fixed Q
free(p);
```

```
void mclBn_precomputedMillerLoop2(
  mclBnGT *f,
  const mclBnG1 *P1, const uint64_t *Q1buf,
  const mclBnG1 *P2, const uint64_t *Q2buf
);
```
- compute `MillerLoop(P1, Q1buf) * MillerLoop(P2, Q2buf)`


```
void mclBn_precomputedMillerLoop2mixed(
  mclBnGT *f,
  const mclBnG1 *P1, const mclBnG2 *Q1,
  const mclBnG1 *P2, const uint64_t *Q2buf
);
```
- compute `MillerLoop(P1, Q2) * MillerLoop(P2, Q2buf)`

## Check value
### Check validness
```
int mclBnFr_isValid(const mclBnFr *x);
int mclBnFp_isValid(const mclBnFp *x);
int mclBnG1_isValid(const mclBnG1 *x);
int mclBnG2_isValid(const mclBnG2 *x);
```
C++
```
bool T::isValid() const;
```
- return 1 if true else 0

### Check the order of a point
```
int mclBnG1_isValidOrder(const mclBnG1 *x);
int mclBnG2_isValidOrder(const mclBnG2 *x);
```
C++
```
bool T::isValidOrder() const;
```

- Check whether the order of `x` is valid or not
- return 1 if true else 0
- This function always checks according to `mclBn_verifyOrderG1` and `mclBn_verifyOrderG2`.

### Is equal / zero / one / isOdd
```
int mclBnFr_isEqual(const mclBnFr *x, const mclBnFr *y);
int mclBnFr_isZero(const mclBnFr *x);
int mclBnFr_isOne(const mclBnFr *x);
int mclBnFr_isOdd(const mclBnFr *x);

int mclBnFp_isEqual(const mclBnFp *x, const mclBnFp *y);
int mclBnFp_isZero(const mclBnFp *x);
int mclBnFp_isOne(const mclBnFp *x);
int mclBnFp_isOdd(const mclBnFp *x);

int mclBnFp2_isEqual(const mclBnFp2 *x, const mclBnFp2 *y);
int mclBnFp2_isZero(const mclBnFp2 *x);
int mclBnFp2_isOne(const mclBnFp2 *x);

int mclBnG1_isEqual(const mclBnG1 *x, const mclBnG1 *y);
int mclBnG1_isZero(const mclBnG1 *x);

int mclBnG2_isEqual(const mclBnG2 *x, const mclBnG2 *y);
int mclBnG2_isZero(const mclBnG2 *x);

int mclBnGT_isEqual(const mclBnGT *x, const mclBnGT *y);
int mclBnGT_isZero(const mclBnGT *x);
int mclBnGT_isOne(const mclBnGT *x);
```
C++
```
bool T::operator==(const T& rhs) const;
bool T::isZero() const;
bool T::isOne() const;
```

- return 1 (true) if true else 0 (false)

### isNegative
```
int mclBnFr_isNegative(const mclBnFr *x);
int mclBnFp_isNegative(const mclBnFr *x);
```
return 1 if x >= half where half = (r + 1) / 2 (resp. (p + 1) / 2).

## Lagrange interpolation

```
int mclBn_FrLagrangeInterpolation(mclBnFr *out, const mclBnFr *xVec, const mclBnFr *yVec, mclSize k);
int mclBn_G1LagrangeInterpolation(mclBnG1 *out, const mclBnFr *xVec, const mclBnG1 *yVec, mclSize k);
int mclBn_G2LagrangeInterpolation(mclBnG2 *out, const mclBnFr *xVec, const mclBnG2 *yVec, mclSize k);
```
- Lagrange interpolation
- recover out = y(0) from {(xVec[i], yVec[i])} for {i=0..k-1}
- return 0 if success else -1
  - satisfy that xVec[i] != 0, xVec[i] != xVec[j] for i != j

```
int mclBn_FrEvaluatePolynomial(mclBnFr *out, const mclBnFr *cVec, mclSize cSize, const mclBnFr *x);
int mclBn_G1EvaluatePolynomial(mclBnG1 *out, const mclBnG1 *cVec, mclSize cSize, const mclBnFr *x);
int mclBn_G2EvaluatePolynomial(mclBnG2 *out, const mclBnG2 *cVec, mclSize cSize, const mclBnFr *x);
```
- Evaluate polynomial
- out = f(x) = c[0] + c[1] * x + ... + c[cSize - 1] * x^{cSize - 1}
- return 0 if success else -1
  - satisfy cSize >= 1

## FAQ
### Why the value set by Fp::setStr is different?
The value set by Fp::setStr is masked and truncated if it is greater than p (resp. r).
See [Set string](api.md#set-string)

### What parameters of configuration are for Ethereum?
mcl supports various mode of hash-to-curve function, serialize/deserialize and getStr/setStr
for historical reasons and backwards compatibility.

If using BLS12-381 and Ethereum compatibility mode, set
```
// C++
Fp::setETHserialization(true);
Fr::setETHserialization(true);
bn::setMapToMode(MCL_MAP_TO_MODE_HASH_TO_CURVE);
```
or
```
// C
mclBn_setETHserialization(1);
mclBn_setMapToMode(MCL_MAP_TO_MODE_HASH_TO_CURVE);
```
and use
```
// C++
void Fp::setBigEndianMod(const uint8_t *x, size_t bufSize);
size_t T::serialize(void *buf, size_t maxBufSize) const
size_t T::deserialize(const void *buf, size_t bufSize);
```
or
```
// C
int mclBnFp_setBigEndianMod(mclBnFp *x, const void *buf, mclSize bufSize);
mclSize mclBnFp_serialize(void *buf, mclSize maxBufSize, const mclBnFp *x);
mclSize mclBnFp_deserialize(mclBnFp *x, const void *buf, mclSize bufSize);
```

Serialization of Fp/Fr
- Fp
  - 48 bytes data in big-endian format
- Fr
  - 32 bytes data in big-endian format
- G1
  - zero : `[0xc0 : (47 bytes zero)]`
  - (x, y) : `d = [48 bytes x]` and `d[0] |= 0x20` if `y < (p+1)/2`.
- G2
  - zero : `[0xc0 : (95 bytes zero)]`
  - (x, y) : `d = [96 bytes x]` and `d[0] |= 0x20` if `b < (p+1)/2` where `y=a+bi`.
