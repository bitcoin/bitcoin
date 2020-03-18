Fuzz-testing Bitcoin Core
==========================

A special test harness in `src/test/fuzz/` is provided for each fuzz target to
provide an easy entry point for fuzzers and the like. In this document we'll
describe how to use it with AFL and libFuzzer.

## Preparing fuzzing

The fuzzer needs some inputs to work on, but the inputs or seeds can be used
interchangeably between libFuzzer and AFL.

Extract the example seeds (or other starting inputs) into the inputs
directory before starting fuzzing.

```
git clone https://github.com/bitcoin-core/qa-assets
export DIR_FUZZ_IN=$PWD/qa-assets/fuzz_seed_corpus
```

AFL needs an input directory with examples, and an output directory where it
will place examples that it found. These can be anywhere in the file system,
we'll define environment variables to make it easy to reference them.

So, only for AFL you need to configure the outputs path:

```
mkdir outputs
export AFLOUT=$PWD/outputs
```

libFuzzer will use the input directory as output directory.

## AFL

### Building AFL

It is recommended to always use the latest version of afl:
```
wget http://lcamtuf.coredump.cx/afl/releases/afl-latest.tgz
tar -zxvf afl-latest.tgz
cd afl-<version>
make
export AFLPATH=$PWD
```

For macOS you may need to ignore x86 compilation checks when running `make`:
`AFL_NO_X86=1 make`.

### Instrumentation

To build Bitcoin Core using AFL instrumentation (this assumes that the
`AFLPATH` was set as above):
```
./configure --disable-shared --enable-tests --enable-fuzz CC=${AFLPATH}/afl-gcc CXX=${AFLPATH}/afl-g++
export AFL_HARDEN=1
make
```

If you are using clang you will need to substitute `afl-gcc` with `afl-clang`
and `afl-g++` with `afl-clang++`, so the first line above becomes:
```
./configure --disable-shared --enable-tests --enable-fuzz CC=${AFLPATH}/afl-clang CXX=${AFLPATH}/afl-clang++
```

We disable ccache because we don't want to pollute the ccache with instrumented
objects, and similarly don't want to use non-instrumented cached objects linked
in.

The fuzzing can be sped up significantly (~200x) by using `afl-clang-fast` and
`afl-clang-fast++` in place of `afl-gcc` and `afl-g++` when compiling. When
compiling using `afl-clang-fast`/`afl-clang-fast++` the resulting
binary will be instrumented in such a way that the AFL
features "persistent mode" and "deferred forkserver" can be used. See
https://github.com/google/AFL/tree/master/llvm_mode for details.

### Fuzzing

To start the actual fuzzing use:

```
export FUZZ_TARGET=bech32  # Pick a fuzz_target
mkdir ${AFLOUT}/${FUZZ_TARGET}
$AFLPATH/afl-fuzz -i ${DIR_FUZZ_IN}/${FUZZ_TARGET} -o ${AFLOUT}/${FUZZ_TARGET} -m52 -- src/test/fuzz/${FUZZ_TARGET}
```

You may have to change a few kernel parameters to test optimally - `afl-fuzz`
will print an error and suggestion if so.

On macOS you may need to set `AFL_NO_FORKSRV=1` to get the target to run.
```
export FUZZ_TARGET=bech32  # Pick a fuzz_target
mkdir ${AFLOUT}/${FUZZ_TARGET}
AFL_NO_FORKSRV=1 $AFLPATH/afl-fuzz -i ${DIR_FUZZ_IN}/${FUZZ_TARGET} -o ${AFLOUT}/${FUZZ_TARGET} -m52 -- src/test/fuzz/${FUZZ_TARGET}
```

## libFuzzer

A recent version of `clang`, the address/undefined sanitizers (ASan/UBSan) and
libFuzzer is needed (all found in the `compiler-rt` runtime libraries package).

To build all fuzz targets with libFuzzer, run

```
./configure --enable-fuzz --with-sanitizers=fuzzer,address,undefined CC=clang CXX=clang++
make
```

See https://llvm.org/docs/LibFuzzer.html#running on how to run the libFuzzer
instrumented executable.

Alternatively, you can run the script through the fuzzing test harness (only
libFuzzer supported so far). You need to pass it the inputs directory and
the specific test target you want to run.

```
./test/fuzz/test_runner.py ${DIR_FUZZ_IN} bech32
```

### macOS hints for libFuzzer

The default clang/llvm version supplied by Apple on macOS does not include
fuzzing libraries, so macOS users will need to install a full version, for
example using `brew install llvm`.

Should you run into problems with the address sanitizer, it is possible you
may need to run `./configure` with `--disable-asm` to avoid errors
with certain assembly code from Bitcoin Core's code. See [developer notes on sanitizers](https://github.com/bitcoin/bitcoin/blob/master/doc/developer-notes.md#sanitizers)
for more information.

You may also need to take care of giving the correct path for clang and
clang++, like `CC=/path/to/clang CXX=/path/to/clang++` if the non-systems
clang does not come first in your path.

Full configure that was tested on macOS Catalina with `brew` installed `llvm`:
```
./configure --enable-fuzz --with-sanitizers=fuzzer,address,undefined CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ --disable-asm
```
