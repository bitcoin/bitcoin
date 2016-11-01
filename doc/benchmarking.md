Benchmarking
============

Litecoin Core has an internal benchmarking framework, with benchmarks
for cryptographic algorithms such as SHA1, SHA256, SHA512 and RIPEMD160. As well as the rolling bloom filter.

Running
---------------------
After compiling Litecoin-core, the benchmarks can be run with:

    src/bench/bench_Litecoin

The output will look similar to:
```
# Benchmark, evals, iterations, total, min, max, median
Base58CheckEncode, 5, 320000, 120.772, 7.49351e-05, 7.59374e-05, 7.54759e-05
Base58Decode, 5, 800000, 122.833, 3.0467e-05, 3.11732e-05, 3.06304e-05
Base58Encode, 5, 470000, 137.094, 5.81061e-05, 5.85109e-05, 5.84462e-05
BenchLockedPool, 5, 530, 34.2023, 0.0128247, 0.0129613, 0.0129026
CCheckQueueSpeedPrevectorJob, 5, 1400, 26.1762, 0.00365048, 0.00388629, 0.00367108
CCoinsCaching, 5, 170000, 48.1074, 5.60229e-05, 5.72316e-05, 5.66214e-05
CoinSelection, 5, 650, 34.6426, 0.0105801, 0.0107699, 0.010664
DeserializeAndCheckBlockTest, 5, 160, 39.2084, 0.0483662, 0.0494199, 0.0490138
DeserializeBlockTest, 5, 130, 23.8129, 0.0357731, 0.0373763, 0.0365858
FastRandom_1bit, 5, 440000000, 38.1609, 1.72974e-08, 1.73882e-08, 1.73478e-08
FastRandom_32bit, 5, 110000000, 72.8237, 1.29992e-07, 1.37014e-07, 1.30115e-07
MempoolEviction, 5, 41000, 89.8883, 0.000432748, 0.000446857, 0.000438483
PrevectorClear, 5, 5600, 47.9229, 0.00169952, 0.0017455, 0.00170315
PrevectorDestructor, 5, 5700, 44.5498, 0.0015561, 0.00156977, 0.00156469
RIPEMD160, 5, 440, 135.988, 0.0615496, 0.062268, 0.0617779
RollingBloom, 5, 1500000, 36.5109, 4.80961e-06, 4.97463e-06, 4.85811e-06
SHA1, 5, 570, 51.808, 0.018065, 0.0182623, 0.0181865
SHA256, 5, 340, 8.31841, 0.00483231, 0.00499803, 0.00485486
SHA256_32b, 5, 4700000, 10.469, 4.43441e-07, 4.47611e-07, 4.45223e-07
SHA512, 5, 330, 33.3408, 0.02017, 0.0202554, 0.0201921
SipHash_32b, 5, 40000000, 38.7088, 1.91103e-07, 1.96998e-07, 1.93792e-07
Sleep100ms, 5, 10, 5.01062, 0.100131, 0.100368, 0.100147
Trig, 5, 12000000, 5.95494, 9.78115e-08, 1.04354e-07, 9.80682e-08
VerifyScriptBench, 5, 6300, 9.02493, 0.000285566, 0.000288433, 0.000286175
```

Help
---------------------
`-?` will print a list of options and exit:

    src/bench/bench_Litecoin -?

Notes
---------------------
More benchmarks are needed for, in no particular order:
- Script Validation
- CCoinDBView caching
- Coins database
- Memory pool
- Wallet coin selection
