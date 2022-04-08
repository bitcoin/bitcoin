# JNI for mcl (experimental)
This library provides functionality to compute the optimal ate pairing
over Barreto-Naehrig (BN) or BLS12-381 curves.

# Initialization
Load the library `mcljava`.
```
import com.herumi.mcl.*;

System.loadLibrary(System.mapLibraryName("mcljava"));
Mcl.SystemInit(curveType); // curveType = Mcl.BN254 or Mcl.BLS12_381
```

# Classes
* `G1` ; The cyclic group instantiated as E(Fp)[r] where where r = p + 1 - t.
* `G2` ; The cyclic group instantiated as the inverse image of E'(Fp^2)[r].
* `GT` ; The cyclic group in the image of the optimal ate pairing.
    * `e : G1 x G2 -> GT`
* `Fr` ; The finite field with characteristic r.

# Methods and Functions
## Fr
* `Fr::setInt(int x)` ; set by x
* `Fr::setStr(String str)` ; set by str such as "123", "0xfff", etc.
* `Fr::setByCSPRNG()` ; randomly set
* `Mcl.neg(Fr y, Fr x)` ; `y = -x`
* `Mcl.add(Fr z, Fr x, Fr y)` ; `z = x + y`
* `Mcl.sub(Fr z, Fr x, Fr y)` ; `z = x - y`
* `Mcl.mul(Fr z, Fr x, Fr y)` ; `z = x * y`
* `Mcl.div(Fr z, Fr x, Fr y)` ; `z = x / y`

## G1

* `Mcl.neg(G1 y, G1 x)` ; `y = -x`
* `Mcl.dbl(G1 y, G1 x)` ; `y = 2x`
* `Mcl.add(G1 z, G1 x, G1 y)` ; `z = x + y`
* `Mcl.sub(G1 z, G1 x, G1 y)` ; `z = x - y`
* `Mcl.mul(G1 z, G1 x, Fr y)` ; `z = x * y`
* `Mcl.verifyOrderG1(boolean doVerify)` ; If set to `true`, `setStr` of `G1` will automatically check subgroup membership.
Set to `false` by default, meaning only a on curve check is done by default.

## G2

* `Mcl.neg(G2 y, G2 x)` ; `y = -x`
* `Mcl.dbl(G2 y, G2 x)` ; `y = 2x`
* `Mcl.add(G2 z, G2 x, G2 y)` ; `z = x + y`
* `Mcl.sub(G2 z, G2 x, G2 y)` ; `z = x - y`
* `Mcl.mul(G2 z, G2 x, Fr y)` ; `z = x * y`
* `Mcl.verifyOrderG2(boolean doVerify)` ; If set to `true`, `setStr` of `G2` will automatically check subgroup membership.
Set to `false` by default, meaning only a on curve check is done by default.

## GT

* `GT::setStr(String str)` ; set by the result of `toString()` method
* `Mcl.mul(GT z, GT x, GT y)` ; `z = x * y`
* `Mcl.pow(GT z, GT x, Fr y)` ; `z = x ^ y`
* `Mcl.inv(GT y, GT x)` ; `y = x^{-1}`

## pairing
* `Mcl.pairing(GT e, G1 P, G2 Q)` ; e = e(P, Q)

# BLS signature sample
```
G2 Q = new G2();
Mcl.hashAndMapToG2(Q, "abc".getBytes());

Fr s = new Fr();
s.setByCSPRNG(); // secret key
G2 pub = new G2();
Mcl.mul(pub, Q, s); // public key = sQ

String m = "signature test";
G1 H = new G1();
Mcl.hashAndMapToG1(H, m.getBytes()); // H = Hash(m)
G1 sign = new G1();
Mcl.mul(sign, H, s); // signature of m = s H

GT e1 = new GT();
GT e2 = new GT();
Mcl.pairing(e1, H, pub); // e1 = e(H, s Q)
Mcl.pairing(e2, sign, Q); // e2 = e(s H, Q);
assertBool("verify signature", e1.equals(e2));
```

# Make test
```
cd ffi/java
make test
```

# Make test for Windows
Install swig and jre at first.
```
cd ffi/java
make_wrap
run-mcl
```

# Sample code
[MclTest.java](https://github.com/herumi/mcl/blob/master/ffi/java/MclTest.java)
