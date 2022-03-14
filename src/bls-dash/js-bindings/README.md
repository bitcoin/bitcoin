## bls-signatures

JavaScript library that implements BLS signatures with aggregation as in [Boneh, Drijvers, Neven 2018](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html), using the relic toolkit for cryptographic primitives (pairings, EC, hashing).

This library is a JavaScript port of the [Chia Network's BLS lib](https://github.com/Chia-Network/bls-signatures). We also have typings, so you can use it with TypeScript too!

### Usage

```bash
npm i bls-signatures --save # or yarn add bls-signatures
```

### Creating keys and signatures
```javascript
  var loadBls = require("bls-signatures");
  var BLS = await loadBls();
  
  var seed = Uint8Array.from([
    0,  50, 6,  244, 24,  199, 1,  25,  52,  88,  192,
    19, 18, 12, 89,  6,   220, 18, 102, 58,  209, 82,
    12, 62, 89, 110, 182, 9,   44, 20,  254, 22
  ]);
  
  var sk = BLS.AugSchemeMPL.key_gen(seed);
  var pk = sk.get_g1();
  
  var message = Uint8Array.from([1,2,3,4,5]);
  var signature = BLS.AugSchemeMPL.sign(sk, message);
  
  let ok = BLS.AugSchemeMPL.verify(pk, message, signature);
  console.log(ok); // true
```

### Serializing keys and signatures to bytes
```javascript  
  var skBytes = sk.serialize();
  var pkBytes = pk.serialize();
  var signatureBytes = signature.serialize();
  
  console.log(BLS.Util.hex_str(skBytes));
  console.log(BLS.Util.hex_str(pkBytes));
  console.log(BLS.Util.hex_str(signatureBytes));
  
```

### Loading keys and signatures from bytes
```javascript
  var skc = BLS.PrivateKey.from_bytes(skBytes, false);
  var pk = BLS.G1Element.from_bytes(pkBytes);

  var signature = BLS.G2Element.from_bytes(signatureBytes);
```

### Create aggregate signatures
```javascript
  // Generate some more private keys
  seed[0] = 1;
  var sk1 = BLS.AugSchemeMPL.key_gen(seed);
  seed[0] = 2;
  var sk2 = BLS.AugSchemeMPL.key_gen(seed);
  var message2 = Uint8Array.from([1,2,3,4,5,6,7]);
  
  // Generate first sig
  var pk1 = sk1.get_g1();
  var sig1 = BLS.AugSchemeMPL.sign(sk1, message);
  
  // Generate second sig
  var pk2 = sk2.get_g1();
  var sig2 = BLS.AugSchemeMPL.sign(sk2, message2);
  
  // Signatures can be non-interactively combined by anyone
  var aggSig = BLS.AugSchemeMPL.aggregate([sig1, sig2]);
  
  ok = BLS.AugSchemeMPL.aggregate_verify([pk1, pk2], [message, message2], aggSig);
  console.log(ok); // true
  
```

### Arbitrary trees of aggregates
```javascript
  seed[0] = 3;
  var sk3 = BLS.AugSchemeMPL.key_gen(seed);
  var pk3 = sk3.get_g1();
  var message3 = Uint8Array.from([100, 2, 254, 88, 90, 45, 23]);
  var sig3 = BLS.AugSchemeMPL.sign(sk3, message3);
  
  var aggSigFinal = BLS.AugSchemeMPL.aggregate([aggSig, sig3]);
  ok = BLS.AugSchemeMPL.aggregate_verify([pk1, pk2, pk3], [message, message2, message3], aggSigFinal);
  console.log(ok); // true
```

### Very fast verification with Proof of Possession scheme
```javascript

  // If the same message is signed, you can use Proof of Posession (PopScheme) for efficiency
  // A proof of possession MUST be passed around with the PK to ensure security.
  var popSig1 = BLS.PopSchemeMPL.sign(sk1, message);
  var popSig2 = BLS.PopSchemeMPL.sign(sk2, message);
  var popSig3 = BLS.PopSchemeMPL.sign(sk3, message);
  var pop1 = BLS.PopSchemeMPL.pop_prove(sk1);
  var pop2 = BLS.PopSchemeMPL.pop_prove(sk2);
  var pop3 = BLS.PopSchemeMPL.pop_prove(sk3);
  
  ok = BLS.PopSchemeMPL.pop_verify(pk1, pop1);
  console.log(ok); // true
  ok = BLS.PopSchemeMPL.pop_verify(pk2, pop2);
  console.log(ok); // true
  ok = BLS.PopSchemeMPL.pop_verify(pk3, pop3);
  console.log(ok); // true
  
  var popSigAgg = BLS.PopSchemeMPL.aggregate([popSig1, popSig2, popSig3]);
  ok = BLS.PopSchemeMPL.fast_aggregate_verify([pk1, pk2, pk3], message, popSigAgg);
  console.log(ok); // true
  
  // Aggregate public key, indistinguishable from a single public key
  var popAggPk = pk1.add(pk2).add(pk3);
  ok = BLS.PopSchemeMPL.verify(popAggPk, message, popSigAgg);
  console.log(ok); // true
  
  // Aggregate private keys
  var aggSk = BLS.PrivateKey.aggregate([sk1, sk2, sk3]);
  ok = (BLS.PopSchemeMPL.sign(aggSk, message).equal_to(popSigAgg));
  console.log(ok); // true
```

### HD keys using [EIP-2333](https://github.com/ethereum/EIPs/pull/2333)
```javascript
  // You can derive 'child' keys from any key, to create arbitrary trees. 4 byte indeces are used.
  // Hardened (more secure, but no parent pk -> child pk)
  var masterSk = BLS.AugSchemeMPL.key_gen(seed);
  var child = BLS.AugSchemeMPL.derive_child_sk(masterSk, 152);
  var grandChild = BLS.AugSchemeMPL.derive_child_sk(child, 952);
  
  // Unhardened (less secure, but can go from parent pk -> child pk), BIP32 style
  var masterPk = masterSk.get_g1();
  var childU = BLS.AugSchemeMPL.derive_child_sk_unhardened(masterSk, 22);
  var grandchildU = BLS.AugSchemeMPL.derive_child_sk_unhardened(childU, 0);
  
  var childUPk = BLS.AugSchemeMPL.derive_child_pk_unhardened(masterPk, 22);
  var grandchildUPk = BLS.AugSchemeMPL.derive_child_pk_unhardened(childUPk, 0);
  
  ok = (grandchildUPk.equal_to(grandchildU.get_g1()));
  console.log(ok); // true
```

Please refer to the library's [typings](./blsjs.d.ts) for detailed API information. Use cases can be found in the [original lib's readme](../README.md).

__Important note on usage:__ Since this library is a WebAssembly port of the c++ library, JavaScript's automatic memory management isn't available. Please, delete all objects manually if they are not needed anymore by calling the delete method on them, as shown in the example below.

```javascript
  sk.delete();
  // ...
  pk.delete();
  // ...
  sig1.delete();
  // ...
```

### Build

Building requires Node.js (with npm) and [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) to be installed.
The build process is the same as for the c++ lib, with one additional step: pass the Emscripten toolchain file as an option to CMake.
From the project root directory, run:
```
#git submodule update --init --recursive
mkdir js_build
cd js_build
cmake ../ -DCMAKE_TOOLCHAIN_FILE={path_to_your_emscripten_installation}/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
cmake --build . --
```

Run the build after any changes to the library, including readme and tests, as the library will be deployed from the build directory, and the build system copies all the files from the source dir.
### Run tests
Tests are run in node.js and Firefox, therefore you need to install node.js and Firefox.
To run tests, build the library, then go to the `js_bindings` folder in the build directory and run
```bash
npm test
```
