## how to build

```
mkdir work
cd work
git clone https://github.com/herumi/mcl
git clone https://github.com/herumi/bls
cd mcl
make src/base64.ll
make BIT=32 src/base32.ll
cd ../bls/ffi/android
ndk-build
```
