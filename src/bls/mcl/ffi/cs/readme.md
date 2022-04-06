# C# binding of mcl library

# How to build `bin/mclbn384_256.dll`.

```
git clone https://github.com/herumi/mcl
cd mcl
mklib dll
```

Open `mcl/ffi/cs/mcl.sln` and Set the directory of `mcl/bin` to `workingDirectory` at `Debug` of test project.

# Remark
- `bn256.cs` is an old code. It will be removed in the future.
- `mcl/mcl.cs` is a new version. It support `BN254`, `BN_SNARK` and `BLS12_381` curve, which requires `mclbn384_256.dll`.

# `ETHmode` with `BLS12_381`

If you need the map-to-G1/G2 function defined in [Hashing to Elliptic Curves](https://www.ietf.org/id/draft-irtf-cfrg-hash-to-curve-09.html),
then initialize this library as the followings:
```
MCL.Init(BLS12_381);
MCL.ETHmode();
```
