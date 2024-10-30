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

    build/src/bench/bench_bitcoin

The output will look similar to:
```
...
|               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | benchmark
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:----------
|          720,914.00 |            1,387.13 |    1.4% |    7,412,886.58 |    3,161,517.17 |  2.345 |   1,021,129.90 |    1.0% |      0.20 | `AddAndRemoveDisconnectedBlockTransactions10`
|          611,867.00 |            1,634.34 |    0.5% |    7,649,982.92 |    2,670,775.17 |  2.864 |   1,056,808.83 |    0.4% |      0.07 | `AddAndRemoveDisconnectedBlockTransactions90`
|          619,097.20 |            1,615.26 |    1.0% |    7,668,842.50 |    2,668,561.30 |  2.874 |   1,059,370.50 |    0.3% |      0.07 | `AddAndRemoveDisconnectedBlockTransactionsAll`
|          159,134.17 |            6,284.01 |    1.3% |    2,071,425.00 |      694,174.43 |  2.984 |     267,128.50 |    0.4% |      0.01 | `AssembleBlock`
...
|               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | benchmark
|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:----------
|            7,188.44 |          139,112.17 |    2.0% |       86,159.12 |       29,225.32 |  2.948 |       8,224.78 |    1.3% |      0.01 | `Linearize16TxWorstCase120Iters`
|            2,030.32 |          492,532.65 |    1.1% |       22,864.15 |        8,028.46 |  2.848 |       2,367.65 |    0.9% |      0.01 | `Linearize16TxWorstCase20Iters`
...
```

Help
---------------------

    build/src/bench/bench_bitcoin -h

To print the various options, like listing the benchmarks without running them
or using a regex filter to only run certain benchmarks.

Notes
---------------------

Benchmarks help with monitoring for performance regressions and act as a scope
for future performance improvements. They should cover components that impact
performance critical functions of the system. Functions are performance
critical, if their performance impacts users and the cost associated with a
degradation in performance is high. A non-exhaustive list:

- Initial block download (Cost: slow IBD results in full node operation being
  less accessible)
- Block template creation (Cost: slow block template creation may result in
  lower fee revenue for miners)
- Block propagation (Cost: slow block propagation may increase the rate of
  orphaned blocks)

Benchmark additions and performance improvements are valuable if they target
the user facing functions mentioned above. If a clear end-to-end performance
improvement cannot be demonstrated, the pull request is likely to be rejected.
The change might also be rejected if the code bloat or review/maintenance is
too much to justify the improvement.

Benchmarks are ill-suited for testing of denial of service issues as they are
restricted to the same input set (introducing bias). [Fuzz
tests](/doc/fuzzing.md) are better suited for this purpose, as they are much
better (and aimed) at exploring the possible input space.

Going Further
--------------------

To monitor Bitcoin Core performance more in depth (like reindex or IBD): https://github.com/chaincodelabs/bitcoinperf

To generate Flame Graphs for Bitcoin Core: https://github.com/eklitzke/bitcoin/blob/flamegraphs/doc/flamegraphs.md
