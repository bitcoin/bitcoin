Benchmarking
============

Bitcoin Core has an internal benchmarking framework, with benchmarks
for cryptographic algorithms (e.g. SHA1, SHA256, SHA512, RIPEMD160, Poly1305, ChaCha20), rolling bloom filter, coins selection,
thread queue, wallet balance.

Running
---------------------

For benchmarking, you only need to compile `bench_bitcoin`.  The bench runner
warns if you configure with `-DCMAKE_BUILD_TYPE=Debug`, but consider if building without
it will impact the benchmark(s) you are interested in by unlatching log printers
and lock analysis.

    cmake -B build -DBUILD_BENCH=ON
    cmake --build build -t bench_bitcoin

After compiling bitcoin-core, the benchmarks can be run with:

    build/bin/bench_bitcoin

The output will look similar to:
```
|               ns/op |                op/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|       57,927,463.00 |               17.26 |    3.6% |      0.66 | `AddrManAdd`
|          677,816.00 |            1,475.33 |    4.9% |      0.01 | `AddrManGetAddr`

...

|             ns/byte |              byte/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|              127.32 |        7,854,302.69 |    0.3% |      0.00 | `Base58CheckEncode`
|               31.95 |       31,303,226.99 |    0.2% |      0.00 | `Base58Decode`

...
```

Help
---------------------

    build/bin/bench_bitcoin -h

To print the various options, like listing the benchmarks without running them
or using a regex filter to only run certain benchmarks.

Notes
---------------------

Benchmarks help with monitoring for performance regressions and can act as a
scope for future performance improvements. They should cover components that
impact performance critical functions of the system. Functions are performance
critical if their performance impacts users and the cost associated with a
degradation in performance is high. A non-exhaustive list:

- Initial block download (Cost: slow IBD results in full node operation being
  less accessible)
- Block template creation (Cost: slow block template creation may result in
  lower fee revenue for miners)
- Block propagation (Cost: slow block propagation may increase the rate of
  orphaned blocks and mining centralization)

A change aiming to improve the performance may be rejected when a clear
end-to-end performance improvement cannot be demonstrated. The change might
also be rejected if the code bloat or review/maintenance burden is too high to
justify the improvement.

Benchmarks are ill-suited for testing denial-of-service issues as they are
restricted to the same input set (introducing bias). [Fuzz
tests](/doc/fuzzing.md) are better suited for this purpose, as they are
specifically aimed at exploring the possible input space.

Going Further
--------------------

To monitor Bitcoin Core performance more in depth (like reindex or IBD): https://github.com/bitcoin-dev-tools/benchcoin
