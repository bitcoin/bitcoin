Benchmarking
============

Bitcoin Core has an internal benchmarking framework, with benchmarks
for cryptographic algorithms (e.g. SHA1, SHA256, SHA512, RIPEMD160, Poly1305, ChaCha20), rolling bloom filter, coins selection,
thread queue, wallet balance.

Running
---------------------

For benchmarks purposes you only need to compile `bitcoin_bench`. Beware of configuring without `--enable-debug` as this would impact
benchmarking by unlatching log printers and lock analysis.

    make -C src bitcoin_bench

After compiling bitcoin-core, the benchmarks can be run with:

    src/bench/bench_bitcoin

The output will look similar to:
```
# Benchmark, evals, iterations, total, min, max, median
AssembleBlock, 5, 700, 1.79954, 0.000510913, 0.000517018, 0.000514497
...
```

Help
---------------------

    src/bench/bench_bitcoin --help

To print options like scaling factor or per-benchmark filter.

Notes
---------------------
More benchmarks are needed for, in no particular order:
- Script Validation
- Coins database
- Memory pool
- Cuckoo Cache
- P2P throughput

Going Further
--------------------

To monitor Bitcoin Core performance more in depth (like reindex or IBD): https://github.com/chaincodelabs/bitcoinperf

To generate Flame Graphs for Bitcoin Core: https://github.com/eklitzke/bitcoin/blob/flamegraphs/doc/flamegraphs.md
