# Fuzzing Bitcoin Core using libFuzzer

## Quickstart guide

To quickly get started fuzzing Bitcoin Core using [libFuzzer](https://llvm.org/docs/LibFuzzer.html):

```sh
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ cmake --preset=libfuzzer
$ cmake --build build_fuzz
$ FUZZ=process_message build_fuzz/bin/fuzz
# abort fuzzing using ctrl-c
```

One can use `--preset=libfuzzer-nosan` to do the same without common sanitizers enabled.
See [further](#run-without-sanitizers-for-increased-throughput) for more information.

There is also a runner script to execute all fuzz targets. Refer to
`./build_fuzz/test/fuzz/test_runner.py --help` for more details.

For source-based coverage reports, see [developer notes](/doc/developer-notes.md#compiling-for-fuzz-coverage).

macOS users: We recommend fuzzing on Linux, see [macOS notes](#macos-notes) for
more information.

## Overview of Bitcoin Core fuzzing

[Google](https://github.com/google/fuzzing/) has a good overview of fuzzing in general, with contributions from key architects of some of the most-used fuzzers. [This paper](https://agroce.github.io/bitcoin_report.pdf) includes an external overview of the status of Bitcoin Core fuzzing, as of summer 2021.  [John Regehr](https://blog.regehr.org/archives/1687) provides good advice on writing code that assists fuzzers in finding bugs, which is useful for developers to keep in mind.

## Fuzzing harnesses and output

[`process_message`](https://github.com/bitcoin/bitcoin/blob/master/src/test/fuzz/process_message.cpp) is a fuzzing harness for the [`ProcessMessage(...)` function (`net_processing`)](https://github.com/bitcoin/bitcoin/blob/master/src/net_processing.cpp). The available fuzzing harnesses are found in [`src/test/fuzz/`](https://github.com/bitcoin/bitcoin/tree/master/src/test/fuzz).

The fuzzer will output `NEW` every time it has created a test input that covers new areas of the code under test. For more information on how to interpret the fuzzer output, see the [libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html).

If you specify a corpus directory then any new coverage increasing inputs will be saved there:

```sh
$ mkdir -p process_message-seeded-from-thin-air/
$ FUZZ=process_message build_fuzz/bin/fuzz process_message-seeded-from-thin-air/
INFO: Seed: 840522292
INFO: Loaded 1 modules   (424174 inline 8-bit counters): 424174 [0x55e121ef9ab8, 0x55e121f613a6),
INFO: Loaded 1 PC tables (424174 PCs): 424174 [0x55e121f613a8,0x55e1225da288),
INFO:        0 files found in process_message-seeded-from-thin-air/
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: A corpus is not provided, starting from an empty corpus
#2      INITED cov: 94 ft: 95 corp: 1/1b exec/s: 0 rss: 150Mb
#3      NEW    cov: 95 ft: 96 corp: 2/3b lim: 4 exec/s: 0 rss: 150Mb L: 2/2 MS: 1 InsertByte-
#4      NEW    cov: 96 ft: 98 corp: 3/7b lim: 4 exec/s: 0 rss: 150Mb L: 4/4 MS: 1 CrossOver-
#21     NEW    cov: 96 ft: 100 corp: 4/11b lim: 4 exec/s: 0 rss: 150Mb L: 4/4 MS: 2 ChangeBit-CrossOver-
#324    NEW    cov: 101 ft: 105 corp: 5/12b lim: 6 exec/s: 0 rss: 150Mb L: 6/6 MS: 5 CrossOver-ChangeBit-CopyPart-ChangeBit-ChangeBinInt-
#1239   REDUCE cov: 102 ft: 106 corp: 6/24b lim: 14 exec/s: 0 rss: 150Mb L: 13/13 MS: 5 ChangeBit-CrossOver-EraseBytes-ChangeBit-InsertRepeatedBytes-
#1272   REDUCE cov: 102 ft: 106 corp: 6/23b lim: 14 exec/s: 0 rss: 150Mb L: 12/12 MS: 3 ChangeBinInt-ChangeBit-EraseBytes-
        NEW_FUNC[1/677]: 0x55e11f456690 in std::_Function_base::~_Function_base() /usr/lib/gcc/x86_64-linux-gnu/8/../../../../include/c++/8/bits/std_function.h:255
        NEW_FUNC[2/677]: 0x55e11f465800 in CDataStream::CDataStream(std::vector<unsigned char, std::allocator<unsigned char> > const&, int, int) src/./streams.h:248
#2125   REDUCE cov: 4820 ft: 4867 corp: 7/29b lim: 21 exec/s: 0 rss: 155Mb L: 6/12 MS: 2 CopyPart-CMP- DE: "block"-
        NEW_FUNC[1/9]: 0x55e11f64d790 in std::_Rb_tree<uint256, std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > >, std::_Select1st<std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > > >, std::less<uint256>, std::allocator<std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > > > >::~_Rb_tree() /usr/lib/gcc/x86_64-linux-gnu/8/../../../../include/c++/8/bits/stl_tree.h:972
        NEW_FUNC[2/9]: 0x55e11f64d870 in std::_Rb_tree<uint256, std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > >, std::_Select1st<std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > > >, std::less<uint256>, std::allocator<std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > > > >::_M_erase(std::_Rb_tree_node<std::pair<uint256 const, std::chrono::duration<long, std::ratio<1l, 1000000l> > > >*) /usr/lib/gcc/x86_64-linux-gnu/8/../../../../include/c++/8/bits/stl_tree.h:1875
#2228   NEW    cov: 4898 ft: 4971 corp: 8/35b lim: 21 exec/s: 0 rss: 156Mb L: 6/12 MS: 3 EraseBytes-CopyPart-PersAutoDict- DE: "block"-
        NEW_FUNC[1/5]: 0x55e11f46df70 in std::enable_if<__and_<std::allocator_traits<zero_after_free_allocator<char> >::__construct_helper<char, unsigned char const&>::type>::value, void>::type std::allocator_traits<zero_after_free_allocator<char> >::_S_construct<char, unsigned char const&>(zero_after_free_allocator<char>&, char*, unsigned char const&) /usr/lib/gcc/x86_64-linux-gnu/8/../../../../include/c++/8/bits/alloc_traits.h:243
        NEW_FUNC[2/5]: 0x55e11f477390 in std::vector<unsigned char, std::allocator<unsigned char> >::data() /usr/lib/gcc/x86_64-linux-gnu/8/../../../../include/c++/8/bits/stl_vector.h:1056
#2456   NEW    cov: 4933 ft: 5042 corp: 9/55b lim: 21 exec/s: 0 rss: 160Mb L: 20/20 MS: 3 ChangeByte-InsertRepeatedBytes-PersAutoDict- DE: "block"-
#2467   NEW    cov: 4933 ft: 5043 corp: 10/76b lim: 21 exec/s: 0 rss: 161Mb L: 21/21 MS: 1 InsertByte-
#4215   NEW    cov: 4941 ft: 5129 corp: 17/205b lim: 29 exec/s: 4215 rss: 350Mb L: 29/29 MS: 5 InsertByte-ChangeBit-CopyPart-InsertRepeatedBytes-CrossOver-
#4567   REDUCE cov: 4941 ft: 5129 corp: 17/204b lim: 29 exec/s: 4567 rss: 404Mb L: 24/29 MS: 2 ChangeByte-EraseBytes-
#6642   NEW    cov: 4941 ft: 5138 corp: 18/244b lim: 43 exec/s: 2214 rss: 450Mb L: 43/43 MS: 3 CopyPart-CMP-CrossOver- DE: "verack"-
# abort fuzzing using ctrl-c
$ ls process_message-seeded-from-thin-air/
349ac589fc66a09abc0b72bb4ae445a7a19e2cd8 4df479f1f421f2ea64b383cd4919a272604087a7
a640312c98dcc55d6744730c33e41c5168c55f09 b135de16e4709558c0797c15f86046d31c5d86d7
c000f7b41b05139de8b63f4cbf7d1ad4c6e2aa7f fc52cc00ec1eb1c08470e69f809ae4993fa70082
$ cat --show-nonprinting process_message-seeded-from-thin-air/349ac589fc66a09abc0b72bb4ae445a7a19e2cd8
block^@M-^?M-^?M-^?M-^?M-^?nM-^?M-^?
```

In this case the fuzzer managed to create a `block` message which when passed to `ProcessMessage(...)` increased coverage.

It is possible to specify `bitcoind` arguments to the `fuzz` executable.
Depending on the test, they may be ignored or consumed and alter the behavior
of the test. Just make sure to use double-dash to distinguish them from the
fuzzer's own arguments:

```sh
$ FUZZ=address_deserialize_v2 build_fuzz/bin/fuzz -runs=1 fuzz_corpora/address_deserialize_v2 --checkaddrman=5 --printtoconsole=1
```

## Fuzzing corpora

The project's collection of seed corpora is found in the [`bitcoin-core/qa-assets`](https://github.com/bitcoin-core/qa-assets) repo.

To fuzz `process_message` using the [`bitcoin-core/qa-assets`](https://github.com/bitcoin-core/qa-assets) seed corpus:

```sh
$ git clone --depth=1 https://github.com/bitcoin-core/qa-assets
$ FUZZ=process_message build_fuzz/bin/fuzz qa-assets/fuzz_corpora/process_message/
INFO: Seed: 1346407872
INFO: Loaded 1 modules   (424174 inline 8-bit counters): 424174 [0x55d8a9004ab8, 0x55d8a906c3a6),
INFO: Loaded 1 PC tables (424174 PCs): 424174 [0x55d8a906c3a8,0x55d8a96e5288),
INFO:      991 files found in qa-assets/fuzz_corpora/process_message/
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: seed corpus: files: 991 min: 1b max: 1858b total: 288291b rss: 150Mb
#993    INITED cov: 7063 ft: 8236 corp: 25/3821b exec/s: 0 rss: 181Mb
…
```

## Using the MemorySanitizer (MSan)

MSan [requires](https://clang.llvm.org/docs/MemorySanitizer.html#handling-external-code)
that all linked code be instrumented. The exact steps to achieve this may vary
but involve compiling `clang` from source, using the built `clang` to compile
an instrumentalized libc++, then using it to build [Bitcoin Core dependencies
from source](../depends/README.md) and finally the Bitcoin Core fuzz binary
itself. One can use the MSan CI job as an example for how to perform these
steps.

Valgrind is an alternative to MSan that does not require building a custom libc++.

## Run without sanitizers for increased throughput

Fuzzing on a harness compiled with `-DSANITIZERS=address,fuzzer,undefined` is
good for finding bugs. However, the very slow execution even under libFuzzer
will limit the ability to find new coverage. A good approach is to perform
occasional long runs without the additional bug-detectors
(`--preset=libfuzzer-nosan`) and then merge new inputs into a corpus as described in
the qa-assets repo
(https://github.com/bitcoin-core/qa-assets/blob/main/.github/PULL_REQUEST_TEMPLATE.md).
Patience is useful; even with improved throughput, libFuzzer may need days and
10s of millions of executions to reach deep/hard targets.

## Reproduce a fuzzer crash reported by the CI

- `cd` into the `qa-assets` directory and update it with `git pull qa-assets`
- locate the crash case described in the CI output, e.g. `Test unit written to
  ./crash-1bc91feec9fc00b107d97dc225a9f2cdaa078eb6`
- make sure to compile with all sanitizers, if they are needed (fuzzing runs
  more slowly with sanitizers enabled, but a crash should be reproducible very
  quickly from a crash case)
- run the fuzzer with the case number appended to the seed corpus path:
  `FUZZ=process_message build_fuzz/bin/fuzz
  qa-assets/fuzz_corpora/process_message/1bc91feec9fc00b107d97dc225a9f2cdaa078eb6`
- If the file does not exist, make sure you are checking out the exact same commit id
  for the qa-assets repo. If the file was found while running the fuzz engine in the CI,
  you should be able to reproduce the crash locally  with the same (or a similar input)
  within a few minutes. Alternatively, you can use the base64 encoded file from the CI log,
  if it exists. e.g.
  `echo "Nb6Fc/97AACAAAD/ewAAgAAAAIAAAACAAAAAoA==" |
  base64 --decode > qa-assets/fuzz_corpora/process_message/1bc91feec9fc00b107d97dc225a9f2cdaa078eb6`

## Submit improved coverage

If you find coverage increasing inputs when fuzzing you are highly encouraged to submit them for inclusion in the [`bitcoin-core/qa-assets`](https://github.com/bitcoin-core/qa-assets) repo.

Every single pull request submitted against the Bitcoin Core repo is automatically tested against all inputs in the [`bitcoin-core/qa-assets`](https://github.com/bitcoin-core/qa-assets) repo. Contributing new coverage increasing inputs is an easy way to help make Bitcoin Core more robust.

## Building and debugging fuzz tests

There are 3 ways fuzz tests can be built:

1. With `-DBUILD_FOR_FUZZING=ON` which forces on fuzz determinism (skipping
   proof of work checks, disabling random number seeding, disabling clock time)
   and causes `Assume()` checks to abort on failure.

   This is the normal way to run fuzz tests and generate new inputs. Because
   determinism is hardcoded on in this build, only the fuzz binary can be built
   and all other binaries are disabled.

2. With `-DBUILD_FUZZ_BINARY=ON -DCMAKE_BUILD_TYPE=Debug` which causes
   `Assume()` checks to abort on failure, and enables fuzz determinism, but
   makes it optional.

   Determinism is turned on in the fuzz binary by default, but can be turned off
   by setting the `FUZZ_NONDETERMINISM` environment variable to any value, which
   may be useful for running fuzz tests with code that deterministic execution
   would otherwise skip.

   Since `BUILD_FUZZ_BINARY`, unlike `BUILD_FOR_FUZZING`, does not hardcode on
   determinism, this allows non-fuzz binaries to coexist in the same build,
   making it possible to reproduce fuzz test failures in a normal build.

3. With `-DBUILD_FUZZ_BINARY=ON -DCMAKE_BUILD_TYPE=Release`. In this build, the
   fuzz binary will build but refuse to run, because in release builds
   determinism is forced off and `Assume()` checks do not abort, so running the
   tests would not be useful. This build is only useful for ensuring fuzz tests
   compile and link.

## macOS notes

Support for fuzzing on macOS is not officially maintained by this project. If
you are running into issues on macOS, we recommend fuzzing on Linux instead for
best results. On macOS this can be done within Docker or a virtual machine.

Reproducing and debugging fuzz testcases on macOS is supported, by building the
fuzz binary without support for any specific fuzzing engine.

# Fuzzing Bitcoin Core using afl++

## Quickstart guide

To quickly get started fuzzing Bitcoin Core using [afl++](https://github.com/AFLplusplus/AFLplusplus):

```sh
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ git clone https://github.com/AFLplusplus/AFLplusplus
$ make -C AFLplusplus/ source-only
# If afl-clang-lto is not available, see
# https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/fuzzing_in_depth.md#a-selecting-the-best-afl-compiler-for-instrumenting-the-target
$ cmake -B build_fuzz \
   -DCMAKE_C_COMPILER="$(pwd)/AFLplusplus/afl-clang-lto" \
   -DCMAKE_CXX_COMPILER="$(pwd)/AFLplusplus/afl-clang-lto++" \
   -DBUILD_FOR_FUZZING=ON
$ cmake --build build_fuzz
$ mkdir -p inputs/ outputs/
$ echo A > inputs/thin-air-input
$ FUZZ=bech32 ./AFLplusplus/afl-fuzz -i inputs/ -o outputs/ -- build_fuzz/bin/fuzz
# You may have to change a few kernel parameters to test optimally - afl-fuzz
# will print an error and suggestion if so.
```

Read the [afl++ documentation](https://github.com/AFLplusplus/AFLplusplus) for more information.

# Fuzzing Bitcoin Core using Honggfuzz

## Quickstart guide

To quickly get started fuzzing Bitcoin Core using [Honggfuzz](https://github.com/google/honggfuzz):

```sh
$ git clone https://github.com/bitcoin/bitcoin
$ cd bitcoin/
$ git clone https://github.com/google/honggfuzz
$ cd honggfuzz/
$ make
$ cd ..
$ cmake -B build_fuzz \
   -DCMAKE_C_COMPILER="$(pwd)/honggfuzz/hfuzz_cc/hfuzz-clang" \
   -DCMAKE_CXX_COMPILER="$(pwd)/honggfuzz/hfuzz_cc/hfuzz-clang++" \
   -DBUILD_FOR_FUZZING=ON \
   -DSANITIZERS=address,undefined
$ cmake --build build_fuzz
$ mkdir -p inputs/
$ FUZZ=process_message ./honggfuzz/honggfuzz -i inputs/ -- build_fuzz/bin/fuzz
```

Read the [Honggfuzz documentation](https://github.com/google/honggfuzz/blob/master/docs/USAGE.md) for more information.

# OSS-Fuzz

Bitcoin Core participates in Google's [OSS-Fuzz](https://github.com/google/oss-fuzz/tree/master/projects/bitcoin-core)
program, which includes a dashboard of [publicly disclosed vulnerabilities](https://issues.oss-fuzz.com/issues?q=bitcoin-core%20status:open).

Bitcoin Core follows its [security disclosure policy](https://bitcoincore.org/en/security-advisories/),
which may differ from Google's standard
[90-day disclosure window](https://google.github.io/oss-fuzz/getting-started/bug-disclosure-guidelines/)
.

OSS-Fuzz also produces [a fuzzing coverage report](https://oss-fuzz.com/coverage-report/job/libfuzzer_asan_bitcoin-core/latest).
