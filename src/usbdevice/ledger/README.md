btchip-c-api
============

C APIs demonstrating how to use the latest BTChip firmware published on http://btchip.github.io/btchip-doc/bitcoin-technical.html

Note : if you're using an older firmware version (before 1.4.7), check the derive-before-1.4.7 branch

Note : Ledger Wallet only supports HIDAPI following version 1.4.14 for HW.1 / Nano, as well as for all versions for Nano S or Blue  

Building on Linux and OS X
---------------------------

Build with GNU make (needs libusb 1.0 development APIs for Makefile.libusb or HID API from Signal 11 for Makefile.hidapi, such as the libhidapi-dev package), each command is described below

Building on Windows
-------------------

Those instructions are outdated and should refer to HID API instead.

  * Download MinGW installer mingw-get from http://sourceforge.net/projects/mingw/files/Installer/
  * Install packages with mingw-get :
  	 * mingw-get install mingw32-autoconf-bin mingw32-automake-bin mingw32-base-bin mingw32-binutils-bin mingw32-gcc-bin mingw32-libstdc++-dll mingw32-libtool-bin mingw32-make-bin msys-autoconf-bin msys-automake-bin msys-base-bin msys-bash-bin msys-binutils-bin msys-core-bin msys-coreutils-bin msys-libtool-bin msys-make-bin
  * Get a binary version of libusb from a stable release : http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.18/libusb-1.0.18-win.7z/download and install it into a libusb/ folder in this repository
  * Compile this project 
     * Start msys
     * Modify the compile path (update your drive reference, might not be necessary) : export PATH=$PATH:/c/mingw/bin
     * Run : make


High level samples
------------------

### Generic transaction 

The generic transaction is available at https://blockchain.info/tx/3ec2589c853dcfa9b3457bd5d45edc1ccc80a9e2525d72c66720a5ec5d56b50f using the BIP32 Seed 1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D and initial transaction https://blockchain.info/tx/104fa062124438e64349c44fa3d17050d0a10b7e3dbafdf0e8da846423da73c7


```
Restore dongle seed

> btchip_setup "WALLET" "RFC6979" "" "" "31323334" "" "QWERTY" "1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D" "" ""


Authenticate

> btchip_verifyPin 31323334

Read the public key

> btchip_getWalletPublicKey "0'/0/0"
Uncompressed public key : 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Address : 17JusYNVXLPm3hBPzzRQkARYDMUBgRUMVc

Compress the public key 

> btchip_util_compressPublicKey 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Compressed public key : 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814

Initialize the transaction

> btchip_startUntrustedTransaction NEW 0 "" "01000000014ea60aeac5252c14291d428915bd7ccd1bfc4af009f4d4dc57ae597ed0420b71010000008a47304402201f36a12c240dbf9e566bc04321050b1984cd6eaf6caee8f02bb0bfec08e3354b022012ee2aeadcbbfd1e92959f57c15c1c6debb757b798451b104665aa3010569b49014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff0281b72e00000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca0860100000000001976a9144533f5fb9b4817f713c48f0bfe96b9f50c476c9b88ac00000000:1"
Trusted input #1
31b3ac57c773da236484dae8f0fdba3d7e0ba1d05070d1a34fc44943e638441262a04f1001000000a0860100000000005c4e6aeeb57363b5
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
Output data : 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac

Powercycle to read the second factor 

>> Powercycle then confirm transfer of 0.0009 BTC to 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC fees 0.0001 BTC change 0 BTC with PIN 9454

Powercycle and compute the signature 

> btchip_startUntrustedTransaction CONTINUE 0 "" "01000000014ea60aeac5252c14291d428915bd7ccd1bfc4af009f4d4dc57ae597ed0420b71010000008a47304402201f36a12c240dbf9e566bc04321050b1984cd6eaf6caee8f02bb0bfec08e3354b022012ee2aeadcbbfd1e92959f57c15c1c6debb757b798451b104665aa3010569b49014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff0281b72e00000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca0860100000000001976a9144533f5fb9b4817f713c48f0bfe96b9f50c476c9b88ac00000000:1"
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
> btchip_untrustedHashSign "0'/0/0" 9454 "" ""
Signature + hashtype : 3046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601

Compute the input script 

> btchip_util_getRegularInputScript 3046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
Input script : 493046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814

Finalize the transaction 

> btchip_util_formatTransaction "" "" 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac 31b3ac57c773da236484dae8f0fdba3d7e0ba1d05070d1a34fc44943e638441262a04f1001000000a0860100000000005c4e6aeeb57363b5 493046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
Transaction : 0100000001c773da236484dae8f0fdba3d7e0ba1d05070d1a34fc44943e638441262a04f10010000006c493046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814ffffffff01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac00000000
```

### P2SH transaction

The P2SH transaction is available at https://blockchain.info/tx/00c20854d077b11133ed5870ea3179574aef3e9338a09ba2dc01f73449781a19 using the BIP32 Seed 1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D for the first dongle, 9C288109F125C1D86DE32FB13FE9AEA77A310FBCAEDBA33BEA6142DB2CA3FADD for the second dongle, and initial transaction https://blockchain.info/fr/tx/c50223b625a94e6c4457cc03d9bded6148f40a537efdbce3aed789d53ec87d60

#### Getting the P2SH address

On the first dongle

```
Restore dongle seed

> btchip_setup "WALLET" "RFC6979" "" "" "31323334" "" "QWERTY" "1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D" "" ""

Authenticate

> btchip_verifyPin 31323334

Read the public key

> btchip_getWalletPublicKey "0'/0/0"
Uncompressed public key : 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Address : 17JusYNVXLPm3hBPzzRQkARYDMUBgRUMVc

Compress the public key 

> btchip_util_compressPublicKey 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Compressed public key : 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
```

On the second dongle

```
Restore dongle seed

> btchip_setup "WALLET" "RFC6979" "" "" "31323334" "" "QWERTY" "9C288109F125C1D86DE32FB13FE9AEA77A310FBCAEDBA33BEA6142DB2CA3FADD" "" ""

Authenticate

> btchip_verifyPin 31323334

Read the public key

> btchip_getWalletPublicKey "0'/0/0"
Uncompressed public key : 045a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d878ba4baf3716af62b317d1e6aa57096b79463fe42eaeaf5238e709c2640354a31
Address : 17yvUbYnbotyu5n1AgH2ChPpJQMmYc62ik

Compress the public key 

> btchip_util_compressPublicKey 045a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d878ba4baf3716af62b317d1e6aa57096b79463fe42eaeaf5238e709c2640354a31
Compressed public key : 035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d87
```

On any dongle 

**Note** : This method is currently disabled - you'll have to compute the P2SH address yourself from the public keys 


```
Compute the P2SH address

> btchip_composeMofNStart 2 2 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814

Powercycle to read the second factor 

>> Powercycle then confirm adding to M of N address 2/2 17JusYNVXLPm3hBPzzRQkARYDMUBgRUMVc with PIN 3389

Continue

> btchip_composeMofNContinue 3389 035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d87

Powercycle to read the second factor 

>> Powercycle then confirm adding to M of N address 2/2 17yvUbYnbotyu5n1AgH2ChPpJQMmYc62ik with PIN 5565

Finalize

> btchip_composeMofNContinue 5565

Powercycle to read the generated address 

>> Generated M of N address 3QzHJGk2gjTocwHCnX6hVS9mtZLMEWz9bf
```

#### Compute the redeem script 

```
> btchip_util_getP2SHRedeemScript 2 2 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814 035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d87
Redeem script : 52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
```

#### Signing on the first dongle

```
Authenticate

> btchip_verifyPin 31323334

Initialize the transaction

> btchip_startUntrustedTransaction NEW 0 "52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae" "0100000002eec06d050fa44ba7352b35bdb7e5444a2de24f8efb7afbc05b4a710fea5eb85a000000008b483045022100f6428a71dea5d5e70a15dd52d1bfddf20b4c2aca734feb0e7d2a1b8965a1aa0e02201bb65259a1e4dabeba279029c385b5b7fcd3617180495b1ba69c5c9851b91a3f014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffffb730054af7141929f99f90a7dd75cbcb83041e1ca70b3c42606a345ded26e9eb010000008c493046022100b303f8134e2949fe371bef1e37ff1a6c94157bd8d2a4a6de4fd4033cfee81b14022100d83d556a00b595e631884a7ef1309fa4097e899ca8257f778a7d26ac457b9593014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff02409c0000000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca08601000000000017a914ff8ecc689b346acc383e92ee32e79a831b4cb7cf8700000000:1"
Trusted input #1
31dbfab9607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000a086010000000000e147194d3911350c
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
Output data : 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac

Powercycle to read the second factor 

>> Powercycle then confirm transfer of 0.0009 BTC to 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC fees 0.0001 BTC change 0 BTC with PIN 9703

Powercycle and compute the signature 

> btchip_startUntrustedTransaction CONTINUE 0 "52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae" "0100000002eec06d050fa44ba7352b35bdb7e5444a2de24f8efb7afbc05b4a710fea5eb85a000000008b483045022100f6428a71dea5d5e70a15dd52d1bfddf20b4c2aca734feb0e7d2a1b8965a1aa0e02201bb65259a1e4dabeba279029c385b5b7fcd3617180495b1ba69c5c9851b91a3f014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffffb730054af7141929f99f90a7dd75cbcb83041e1ca70b3c42606a345ded26e9eb010000008c493046022100b303f8134e2949fe371bef1e37ff1a6c94157bd8d2a4a6de4fd4033cfee81b14022100d83d556a00b595e631884a7ef1309fa4097e899ca8257f778a7d26ac457b9593014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff02409c0000000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca08601000000000017a914ff8ecc689b346acc383e92ee32e79a831b4cb7cf8700000000:1"
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
> btchip_untrustedHashSign "0'/0/0" 9703 "" ""
Signature + hashtype : 3045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501
```

#### Signing on the second dongle

```
Authenticate

> btchip_verifyPin 31323334

Initialize the transaction

> btchip_startUntrustedTransaction NEW 0 "52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae" "0100000002eec06d050fa44ba7352b35bdb7e5444a2de24f8efb7afbc05b4a710fea5eb85a000000008b483045022100f6428a71dea5d5e70a15dd52d1bfddf20b4c2aca734feb0e7d2a1b8965a1aa0e02201bb65259a1e4dabeba279029c385b5b7fcd3617180495b1ba69c5c9851b91a3f014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffffb730054af7141929f99f90a7dd75cbcb83041e1ca70b3c42606a345ded26e9eb010000008c493046022100b303f8134e2949fe371bef1e37ff1a6c94157bd8d2a4a6de4fd4033cfee81b14022100d83d556a00b595e631884a7ef1309fa4097e899ca8257f778a7d26ac457b9593014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff02409c0000000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca08601000000000017a914ff8ecc689b346acc383e92ee32e79a831b4cb7cf8700000000:1"
Trusted input #1
317eb06f607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000a08601000000000032ca0b82c10d708b
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
Output data : 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac

Powercycle to read the second factor 

>> Powercycle then confirm transfer of 0.0009 BTC to 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC fees 0.0001 BTC change 0 BTC with PIN 6201

Powercycle and compute the signature 

> btchip_startUntrustedTransaction CONTINUE 0 "52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae" "0100000002eec06d050fa44ba7352b35bdb7e5444a2de24f8efb7afbc05b4a710fea5eb85a000000008b483045022100f6428a71dea5d5e70a15dd52d1bfddf20b4c2aca734feb0e7d2a1b8965a1aa0e02201bb65259a1e4dabeba279029c385b5b7fcd3617180495b1ba69c5c9851b91a3f014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffffb730054af7141929f99f90a7dd75cbcb83041e1ca70b3c42606a345ded26e9eb010000008c493046022100b303f8134e2949fe371bef1e37ff1a6c94157bd8d2a4a6de4fd4033cfee81b14022100d83d556a00b595e631884a7ef1309fa4097e899ca8257f778a7d26ac457b9593014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff02409c0000000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca08601000000000017a914ff8ecc689b346acc383e92ee32e79a831b4cb7cf8700000000:1"
> btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
> btchip_untrustedHashSign "0'/0/0" 6201 "" ""
Signature + hashtype : 3045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e01
```

#### Finalize the transaction

```
Compute the input script 

> btchip_util_getP2SHInputScript 52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae 3045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501 3045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e01
Input script : 00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae

Finalize the transaction 

> btchip_util_formatTransaction "" "" 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac 317eb06f607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000a08601000000000032ca0b82c10d708b 00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
Transaction : 0100000001607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000db00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752aeffffffff01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac00000000
```

Initialization commands
------------------------

### btchip_setup

This command is used to initialize a dongle running the latest firmware version

Run with the following parameters :

  * Operation mode flags combined with + (WALLET|RELAXED|SERVER|DEVELOPER)]
  * Features flag combined with + (UNCOMPRESSED_KEYS|RFC6979|FREE_SIGHASHTYPE|NO_2FA_P2SH)
  * hex key version (1 byte or empty for bitcoin mainnet)
  * hex key version P2SH (1 byte or empty for bitcoin mainnet)
  * hex user pin (min 4 bytes)
  * hex wipe pin (or empty)
  * keymap encoding (QWERTY or AZERTY or 119 bytes)
  * seed (32 bytes or empty to generate a new one)
  * user entropy (32 bytes or empty, must be empty if restoring a seed)
  * developer key (16 bytes or empty)

After running, power cycle if generating a new seed to read it using the second factor mechanism.

Sample:
```
btchip_setup "WALLET" "RFC6979" "" "" "31323334" "" "QWERTY" "" "" ""
```

### btchip_setup_143

This command is used to initialize a dongle running the early 1.4.3 firmware version and is **deprecated**

Run with the following parameters :

  * Operation mode flags combined with + (WALLET|RELAXED|SERVER|DEVELOPER)]
  * Features flag combined with + (UNCOMPRESSED_KEYS|RFC6979|FREE_SIGHASHTYPE|NO_2FA_P2SH)
  * hex key version (1 byte or empty for bitcoin mainnet)
  * hex key version P2SH (1 byte or empty for bitcoin mainnet)
  * hex user pin (min 4 bytes)
  * hex wipe pin (or empty)
  * keymap encoding (QWERTY or AZERTY or 119 bytes)
  * seed (32 bytes or empty to generate a new one)
  * user entropy (32 bytes or empty, must be empty if restoring a seed)
  * developer key (16 bytes or empty)

Sample:
```
btchip_setup_143 "WALLET" "RFC6979" "" "" "31323334" "" "QWERTY" "" "" ""
```

### btchip_setup_forward

This command is used to pre-initialize a dongle in a vending machine scenario - a seed is generated on the dongle and can only be read after the provided user PIN is issued.

Run with the following parameters :

  * Operation mode flags combined with + (WALLET|RELAXED|SERVER|DEVELOPER)]
  * Features flag combined with + (UNCOMPRESSED_KEYS|RFC6979|FREE_SIGHASHTYPE|NO_2FA_P2SH)
  * hex key version (1 byte or empty for bitcoin mainnet)
  * hex key version P2SH (1 byte or empty for bitcoin mainnet)
  * hex end user public key
  * hex end user password blob
  * user entropy (32 bytes)

The password blob is generated as described in the firmware documentation.

A sample is provided for the following test conditions :

  * User private key : b08b9492b734c062cc44ec76bc3209b12789a1572fc0fd2dd34108f8e8d77d6a
  * User PIN : 31323334
  * ECDH shared secret (for validation purposes, performed with the first attestation public key listed below) : E204FA08D6C68047E8418926134F032AFEA0722B6BEDE96CA7A32AB2CBF22AD8 

```
btchip_setup_forward "WALLET" "RFC6979" "" "" "048784A3FD7DE7C5E31345867954E855BE254A4F7E467876987F5F8E0D00670B80817EF12F4AE32C4E8248E7A529C8665000E9794DE284A3E0D72575C5FB64797E" "31EC339BB735955E881F490A06D4563D1590A116240776C998442F3318F8CA457DB96C294B1F5A71" ""
```

Lifecycle commands
-------------------

### btchip_verifyPin

This command is used to provide the user PIN 

Run with the following parameters : 

  * Hexadecimal PIN

Sample:
```
btchip_verifyPin "31323334"
```

### btchip_getOperationMode

This command is used to get the current operation mode 

Sample:
```
btchip_getOperationMode
```

### btchip_setOperationMode

This command is used to set the current operation mode 

Run with the following parameters :

  * Operation mode (WALLET|RELAXED|SERVER|DEVELOPER)

Sample:
```
btchip_setOperationMode "WALLET"
```

### btchip_setTransportWinUSB

This command is used to set the dongle communication mode to WinUSB (default)

The USB device will be seen as 2581:1b7c

Sample:
```
btchip_setTransportWinUSB
```

### btchip_setTransportHID

This command is used to set the dongle communication mode to Generic HID

The USB device will be seen as 2581:2b7c

Sample:
```
btchip_setTransportHID
```

Generic wallet commands
------------------------

The samples given below have been generated for the following parameters :

  * BIP 32 Seed : 1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D
  * Trusted input key : 00f34a2f1dbdeec43d98a6119ab07954
  * Developer key wrapping key : 1a48b60969810d8d98d6d469118a42f5
  * Incoming transaction : https://blockchain.info/tx/104fa062124438e64349c44fa3d17050d0a10b7e3dbafdf0e8da846423da73c7

### btchip_getWalletPublicKey

This command is used to retrieve a public key managed by the dongle 

Run with the following parameters : 

  * BIP 32 derivation path

Sample:
```
btchip_getWalletPublicKey "0'/0/0"
Uncompressed public key : 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Address : 17JusYNVXLPm3hBPzzRQkARYDMUBgRUMVc
```

### btchip_getTrustedInput

This command is used to get a future trusted input for a given transaction output - it is provided for reference as the startUntrustedTransaction performs this operation under the hood

Run with the following parameters:

  * Hex transaction:output index

Sample:
```
btchip_getTrustedInput "01000000014ea60aeac5252c14291d428915bd7ccd1bfc4af009f4d4dc57ae597ed0420b71010000008a47304402201f36a12c240dbf9e566bc04321050b1984cd6eaf6caee8f02bb0bfec08e3354b022012ee2aeadcbbfd1e92959f57c15c1c6debb757b798451b104665aa3010569b49014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff0281b72e00000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca0860100000000001976a9144533f5fb9b4817f713c48f0bfe96b9f50c476c9b88ac00000000:1"
Trusted input : 311cccdac773da236484dae8f0fdba3d7e0ba1d05070d1a34fc44943e638441262a04f1001000000a08601000000000034e62587c22992d5
```

### btchip_startUntrustedTransaction

This command is used to initiate a transaction signature for a given input

Run with the following parameters:

  * NEW for a new transaction|CONTINUE to keep on signing inputs in a previous transaction
  * index of input to sign
  * redeem script to use for this input (or empty to use the default one) - this is only used for multisignature transactions
  * list of transactions outputs to use in this transaction (coded as transaction:output index to generate a trusted input or given the hex prevout for an output not owned by the user in relaxed wallet mode)

Sample:
```
btchip_startUntrustedTransaction NEW 0 "" "01000000014ea60aeac5252c14291d428915bd7ccd1bfc4af009f4d4dc57ae597ed0420b71010000008a47304402201f36a12c240dbf9e566bc04321050b1984cd6eaf6caee8f02bb0bfec08e3354b022012ee2aeadcbbfd1e92959f57c15c1c6debb757b798451b104665aa3010569b49014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff0281b72e00000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca0860100000000001976a9144533f5fb9b4817f713c48f0bfe96b9f50c476c9b88ac00000000:1"
```

### btchip_finalizeInput

This command is used to finalize a transaction signature for a given input using a simple output to a single address.

Run with the following parameters:

  * Output address
  * Amount (in BTC string)
  * Fees (in BTC string)
  * BIP 32 derivation path for the change address
  
The change parameters are ignored if no change is generated.

When signing the first input, the dongle must be powercycled to read the transaction confirmation PIN using the second factor

Sample:
```
btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
Output script : 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac
```

### btchip_finalizeInputFull

This command is used to finalize a transaction signature for a given input using an arbitrary output. This method is only available in relaxed wallet mode.

Run with the following parameters:

  * Output data

When signing the first input, the dongle must be powercycled to read the transaction confirmation PIN using the second factor

Sample:
```
btchip_finalizeInputFull 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac
```

### btchip_untrustedHashSign

This command is used to perform the transaction signature for a given input

**Note** : If this command is sent for the first signed input and a second factor was requested, the transaction must be reprocessed first before issuing it.

Run with the following parameters: 

  * BIP 32 derivation path
  * Second factor in ASCII format (or empty string in server mode)
  * Locktime (or empty string for the default locktime)
  * sigHashType (or empty string for SIGHASH_ALL)

Sample (for the first call):
```
btchip_startUntrustedTransaction CONTINUE 0 "01000000014ea60aeac5252c14291d428915bd7ccd1bfc4af009f4d4dc57ae597ed0420b71010000008a47304402201f36a12c240dbf9e566bc04321050b1984cd6eaf6caee8f02bb0bfec08e3354b022012ee2aeadcbbfd1e92959f57c15c1c6debb757b798451b104665aa3010569b49014104090b15bde569386734abf2a2b99f9ca6a50656627e77de663ca7325702769986cf26cc9dd7fdea0af432c8e2becc867c932e1b9dd742f2a108997c2252e2bdebffffffff0281b72e00000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88aca0860100000000001976a9144533f5fb9b4817f713c48f0bfe96b9f50c476c9b88ac00000000:1"
btchip_finalizeInput 1BTChipvU14XH6JdRiK9CaenpJ2kJR9RnC 0.0009 0.0001 "0'/1/0"
btchip_untrustedHashSign "0'/0/0" "1234" "" "" #replace with your second factor
Signature + hashtype : 3046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601
```

### btchip_signMessage

This command set is used to sign a message using a private key owned by the dongle

#### btchip_signMessagePrepare

This command is used to initialize message signing

Run with the following parameters: 

  * BIP 32 derivation path
  * Message to sign

Following this command, the dongle must be powercycled to read the confirmation PIN using the second factor

Sample:
```
btchip_signMessagePrepare "0'/0/0" "Campagne de Sarkozy : une double comptabilite chez Bygmalion"
```

#### btchip_signMessageSign

This command is used to finalize message signing and read the signature

Run with the following parameters:

  * Second factor in ASCII format (or empty string in server mode)

Sample:
```
btchip_signMessageSign 1234
Signature : 30450221009a0d28391c0535aec1077bbb86614c8f3c384a3e9aa1a124bfb9ce9649196b7e02200efa1adc010a7bdde4784ee98441e402f93b3c50a2760cb09dda07501e02c81f
```

Developer commands
------------------

### btchip_importPrivateKey

This command is used to import a BIP32 seed or a standalone private key 

Run with the following parameters:

  * Imported data format (BASE58 or BIP32SEED)
  * Private key encoded ascii or hex encoded seed

Sample:
```
btchip_importPrivateKey BIP32SEED 1762F9A3007DBC825D0DD9958B04880284E88F10C57CF569BB3DADF7B1027F2D
Encoded private key : 0102009170a3e8ba7543269ceb2c657640bdff8d0a1524461989d4cb4a41f21b43952ee9fbdca8ce84424cd3c7494f16815deda0498e2ab83ef6f676cf8f578e838fd4000000000000000000
```

### btchip_deriveBip32Key

This command is used to derive an encoded BIP32 private key on the given index

Run with the following parameters:

  * Index (hex encoded)

Sample to retrieve the 0'/0/0 key:
```
btchip_deriveBip32Key 0102009170a3e8ba7543269ceb2c657640bdff8d0a1524461989d4cb4a41f21b43952ee9fbdca8ce84424cd3c7494f16815deda0498e2ab83ef6f676cf8f578e838fd4000000000000000000 80000000
Encoded private key : 010200a33ff79e7e201d9413395c8573b89677693d64772ebb39ff48a4b19682b6f632b51a98fc30156559ae5f2456ecdde38a7623a9fae0dd4363b0589ab8d1dbddcd01cc217cc280000000

bin/btchip_deriveBip32Key 010200a33ff79e7e201d9413395c8573b89677693d64772ebb39ff48a4b19682b6f632b51a98fc30156559ae5f2456ecdde38a7623a9fae0dd4363b0589ab8d1dbddcd01cc217cc280000000 0
Encoded private key : 01020019d89329930eda3316395cd0a5db549c8c819173346b089627921c7a221ab87e3711ad425cb19b369ba354710c9459424296d64bd556859ebff1b86020a8a6f502e9a0c54c00000000

bin/btchip_deriveBip32Key 01020019d89329930eda3316395cd0a5db549c8c819173346b089627921c7a221ab87e3711ad425cb19b369ba354710c9459424296d64bd556859ebff1b86020a8a6f502e9a0c54c00000000 0
Encoded private key : 010200bac6c49d6da91453c76baf54ba4f14d68fc85775ca837ddcfde07ecc6f437bb762d7482ba2f282d008153549944670d8df898b69124b51ad5a928a9d348af83903aee82af800000000
```

### btchip_getPublicKey

This command is used to return the public key associated to an encoded private key

Run with the following parameters:

  * Encoded private key

Sample to retrieve the 0'/0/0 key:
```
bin/btchip_getPublicKey 010200bac6c49d6da91453c76baf54ba4f14d68fc85775ca837ddcfde07ecc6f437bb762d7482ba2f282d008153549944670d8df898b69124b51ad5a928a9d348af83903aee82af800000000
Public key : 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Chain code : f3ca72087632649b8ecdf48eba73524a77a599b704fa8dfb120ee032f4935273
Depth : 3
Parent fingerprint : aee82af8
Child number : 00000000
```

### btchip_signImmediate

This command is used to sign a hash using an encoded private key

Run with the following parameters:

  * Random mode (RANDOM or DETERMINISTIC signature)
  * Encoded private key
  * Hash to sign

Sample:
```
btchip_signImmediate DETERMINISTIC 010200bac6c49d6da91453c76baf54ba4f14d68fc85775ca837ddcfde07ecc6f437bb762d7482ba2f282d008153549944670d8df898b69124b51ad5a928a9d348af83903aee82af800000000 e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
Signature : 304502205130231bc7556f87e193049dc94f0729b4bfbdaa4318745b86c08138fd9dd9e502210090d1fe71a3760652752421edfb1865728c5578266bcc9c435d850c303f55f1e7
```

### btchip_verifyImmediate

This command is used to verify a signature

Run with the following parameters:

  * Uncompressed public key
  * Hash to sign
  * Signature to verify

Sample:
```
btchip_verifyImmediate 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447 e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855 304502205130231bc7556f87e193049dc94f0729b4bfbdaa4318745b86c08138fd9dd9e502210090d1fe71a3760652752421edfb1865728c5578266bcc9c435d850c303f55f1e7
Signature verified.
```

Utility commands
----------------

### btchip_getFirmwareVersion

This command is used to retrieve the current firmware version

Sample:
```
btchip_getFirmwareVersion
Firmware version 1.4.4
Using compressed keys : yes
```

### btchip_getRandom

This command is used to verify the onboard random number generator

Run with the following parameters:

  * Number of random bytes to get

Sample:
```
btchip_getRandom 32
Random : 2535fc2ea6f59ef81bd837d000beabe0bd6aa726953e863c948a3c0de2f3ddfb
```

### btchip_composeMofN

This command set is used to generate a P2SH M of N multisignature address

#### btchip_composeMofNStart

**Note**: This method is currently disabled

This command is used to start building the P2SH address

Run with the following parameters:

  * M
  * N
  * Public key 

Following this command, the dongle must be powercycled to read the confirmation PIN using the second factor

Sample:
```
btchip_composeMofNStart 2 2 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
```

#### btchip_composeMofNContinue

**Note** : This method is currently disabled

This command is used to keep on building the P2SH address

Run with the following parameters:

  * Second factor in ASCII format
  * Public key

Following this command, the dongle must be powercycled to read the confirmation PIN or the generated P2SH address using the second factor

Sample:
```
btchip_composeMofNContinue 1234 035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d87
btchip_composeMofNContinue 1234
```

External utilities
------------------

### btchip_util_compressPublicKey

This command is used to compress a public key

Run with the following parameters :

  * Uncompressed public key 

Sample:
```
btchip_util_compressPublicKey 0448bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814de82901db19a1fd442e0ebcefd4cfe7316f27aada8cd36b69d24e0d98ba99447
Compressed public key : 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
```

### btchip_util_getP2SHRedeemScript

This command is used to get the P2SH redeem script for a multisignature address

Run with the following parameters :

  * M
  * N
  * List of public keys 

Sample:
```
btchip_util_getP2SHRedeemScript 2 2 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814 035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d87
Redeem script : 52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
```

### btchip_util_getRegularInputScript

This command is used to get a transaction input script for a transaction using a regular input

Run with the following parameters : 

  * Signature followed by hashtype byte
  * Public key

Sample:
```
btchip_util_getRegularInputScript 3046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601 0348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
Input script : 493046022100ea6df031b47629590daf5598b6f0680ad0132d8953b401577f01e8cc46393fe6022100ddfe485e628f95fdec230148fcc8e6458e3f075c822b4cfaa1eb423f4ce32bf601210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d5814
```

### btchip_util_getP2SHInputScript

This command is used to get a transaction input for a transaction using a P2SH input

Run with the following parameters :

  * Redeem script
  * List of signatures followed by hashtype byte

Sample:
```
btchip_util_getP2SHInputScript 52210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae 3045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501 3045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e01
Input script : 00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
```  

### btchip_util_formatTransaction

This command is used to put together the pieces of a transaction and return the serialized version ready to be broadcasted

Run with the following parameters : 

  * Version (or empty for the default version)
  * Locktime (or empty for the default locktime)
  * Output data returned by the dongle
  * List of (trusted input, input script) pairs

Sample:
```
btchip_util_formatTransaction "" "" 01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac 31602ff4607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000a0860100000000008d2682fe8c94f612 00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
Transaction : 0100000001607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000db00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752aeffffffff01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac00000000
```

### btchip_util_parseRawTransaction

This command is used to debug a transaction by returning a human readable form 

Run with the following parameters:

  * Raw transaction

Sample:
```
btchip_util_parseRawTransaction 0100000001607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000db00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752aeffffffff01905f0100000000001976a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac00000000
Version : 01000000
        Input #1 :
                Prevout : 607dc83ed589d7aee3bcfd7e530af44861edbdd903cc57446c4ea925b62302c501000000
                Sequence : ffffffff
                Script : 00483045022100cf9e9f53a88664cf9de94fc7a83cbf2e644f44d8cb9f19589ccc4d6df0cec7e60220321ae27a46ea52719de0176151ed8f98c9de96172779b2d65f6d4baaf4c37c9501483045022100db2b29ef23da6a32fca0f33541f8532cc5c1b097fcf09d6836eaf0cdc2a5cf3202201be4a63502df6697237b1194f59e4b0c1d851c82bc299e114906215040f68f4e014752210348bb1fade0adde1bf202726e6db5eacd2063fce7ecf8bbfd17377f09218d581421035a86ac207f84ef5a42878576ba174f27e494c5a6acaa4592a4a007cb54525d8752ae
        Output #1 :
                Amount : 0.0009 BTC - 905f010000000000
                Script : 76a91472a5d75c8d2d0565b656a5232703b167d50d5a2b88ac
Locktime : 00000000
```

### btchip_util_runScript

This command is used to run an APDU script for the dongle - it is provided for local firmware updates

Run with the following parameters : 

  * Script to run

Sample:
```
btchip_util_runScript script.txt
```

Attestation public keys
-----------------------

### Batch ID 01, Derivation index ID 01

The following attestation public key is used for test samples and production cards before 1.4.11

```
04223314cdffec8740150afe46db3575fae840362b137316c0d222a071607d61b2fd40abb2652a7fea20e3bb3e64dc6d495d59823d143c53c4fe4059c5ff16e406
```

### Batch ID 02, Derivation index ID 01

The following attestation public key is used for production cards from 1.4.11 

```
04c370d4013107a98dfef01d6db5bb3419deb9299535f0be47f05939a78b314a3c29b51fcaa9b3d46fa382c995456af50cd57fb017c0ce05e4a31864a79b8fbfd6
```

License
--------

All code is licensed under the Apache License, Version 2.0.
