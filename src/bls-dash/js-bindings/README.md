## bls-signatures

JavaScript library that implements BLS signatures with aggregation as in [Boneh, Drijvers, Neven 2018](https://crypto.stanford.edu/~dabo/pubs/papers/BLSmultisig.html), using the relic toolkit for cryptographic primitives (pairings, EC, hashing).

This library is a JavaScript port of the [Chia Network's BLS lib](https://github.com/Chia-Network/bls-signatures). We also have typings, so you can use it with TypeScript too!

### Usage

```bash
npm i bls-signatures --save
```
```javascript
const blsSignatures = require('bls-signatures')();
blsSignatures.then(() => {
    // Instance is loaded and ready to be used
    const { PrivateKey } = blsSignatures;
    const privateKey = PrivateKey.fromSeed(Uint8Array.from([1,2,3]));
    const sig = privateKey.sign(Uint8Array.from(Buffer.from("Hello world!")));
    const isValidSignature = sig.verify();
    
    if (isValidSignature) {
        // Do stuff...
    }
    
    privateKey.delete();
    sig.delete();
})
```

Please refer to the library's [typings](./blsjs.d.ts) for detailed API information. Use cases can be found in the [original lib's readme](../README.md).

__Important note on usage:__ Since this library is a WebAssembly port of the c++ library, JavaScript's automatic memory management isn't available. Please, delete all objects manually if they are not needed anymore by calling the delete method on them, as shown in the example above.

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
