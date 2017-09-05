Release Notes for Bitcoin Unlimited Cash Edition 1.1.1.1
=========================================================

Bitcoin Unlimited Cash Edition version 1.1.1.1 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is a minor release version based of Bitcoin Unlimited compatible
with the Bitcoin Cash specification you could find here:

https://github.com/Bitcoin-UAHF/spec/blob/master/uahf-technical-spec.md

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

The main change of this release is related to fixing a peering problem we had due to
BUcash forwarding transactions that were not proper Bitcoin Cash transactions (see `be83d56`).

Other notable changes:

- Update LevelDB to 1.20
- Update windows build doc
- Make the QR code show a fork specific prefix. I.e. use `bitcoincash:` rather than `bitcoin:` in payment URI
- Optimize LevelDB flushing/pruning
- Parallel executions python integration tests suite

Commit details
--------------

- `28d94f533` Make sure we set the pickedForkId flag if the user didn't specify (Peter Tschipper)
- `c2c9363b4` remove unused code (Andrew Stone)
- `bd491e666` Do not process INV, GETDATA or GETBLOCKS when fReindex or fImporting (Peter Tschipper)
- `cc6513253` Create smaller consistently size batches when flushing to leveldb (Peter Tschipper)
- `3de379138` 1/8 Only flush and delete pruned entries 1/(Peter Tschipper)
- `3de379138` 2/8 Don't process INV messages when reindexing 2/(Peter Tschipper)
- `3de379138` 3/8 When doing IBD or reindexing then trim the dbcache by 10% 3/(Peter Tschipper)
- `3de379138` 4/8 update max dbcache to 2GB and default dbcache to 500MB 4/(Peter Tschipper)
- `3de379138` 5/8 Flush the dbcache at least every 512MB increase 5/(Peter Tschipper)
- `3de379138` 6/8 Do not delete coins when in BlocksOnly mode 6/(Peter Tschipper)
- `3de379138` 7/8 Only print to the log if nTrimmed > 0 (Peter Tschipper)
- `3de379138` 8/8 Never Trim more than nMaxCacheIncreaseSinceLastFlush (Peter Tschipper)
- `679284855` Do not allow users to get keys from keypool without reserving them (Matt Corallo)
- `3ce392546` 1.1.1.1 point version (G. Andrew Stone)
- `77c14e7cd` Temporarily do not ban BitcoinCash BU nodes (Peter Tschipper)
- `6f6b084d0` Renable the use of mocktime (Peter Tschipper)
- `ef7f62591` use BITCOIN_CASH define to determine which ui string to return (Peter Tschipper)
- `46fed47f3` Remove #include Application.h (Peter Tschipper)
- `4a276240c` Make the QR code show a fork specific prefix (TomZ)
- `11a3c8de0` use default nHashType SIGHASH_FORKID when signing (Peter Tschipper)
- `8e7625f96` Fix two regression tests (Peter Tschipper)
- `46e4008e3` Fix signrawtransaction which was causing many regression test failures (Peter Tschipper)
- `be83d5646` Be more restrictive in what txns are considered valid (Peter Tschipper)
- `99cfe5237` Port of Core's #10958: Update to latest Bitcoin patches for LevelDB (Wladimir J. van der Laan)
- `2da740663` Port of Core's #10806: build: verify that the assembler can handle crc32 functions (Wladimir J. van der Laan)
- `635867f8b` Add leveldb crc32 lib to fuzzy test framework ldflags (Andrea Suisani)
- `521cfbf73` Port Core #10544: Update to LevelDB 1.20 (Wladimir J. van der Laan)
- `c7069e6bc` Use `os.path.join` rather than string concatenation (Andrea Suisani)
- `5b2ae4266` [qa] Print debug info on stdout rather than stderr (Andrea Suisani)
- `a42ac88bb` [qa] pull-tester: Fix assertion and check for run_parallel (MarcoFalke)
- `a761f3231` [qa] Adapt BU final test summary to the parallel execution (Andrea Suisani)
- `8106a12c8` [qa] Print debug info on stdout rather than stderr (Andrea Suisani)
- `32ed8fe43` [qa] rpc-tests: Apply random offset to portseed (MarcoFalke)
- `37eba151a` [qa] test_framework: Append portseed to tmpdir (MarcoFalke)
- `03a77fe8f` [qa] Add option --portseed to test_framework (MarcoFalke)
- `39cf113d1` [qa] pull-tester: Run rpc test in parallel (MarcoFalke)
- `c800f7ab7` [qa] test_framework: Exit when tmpdir exists (MarcoFalke)
- `9ace0e950` [qa] Refactor test_framework and pull tester (MarcoFalke)
- `1c602d221` [qa] Stop other nodes, even when one fails to stop (MarcoFalke)
- `2f4132610` Autofind rpc tests --srcdir (Jonas Schnelli)
- `1a7f19b95` Fix spelling of `-rpcservertimeout` in pruning.py (Andrea Suisani)
- `0c2f82600` Add BUcash 1.1.0 and 1.1.1 to `dev` branch (Andrea Suisani)
- `85f212484` Overly verbose keypool reserve/return traces in log (Andrea Suisani)
- `8f1467857` [Doc] Update Windows build doc for CVE-2017-8798 (Justaphf)
- `cb9280e95` Fix commits list format (Andrea Suisani)
- `3e5a26ec7` fix bug with fundrawtransaction that ends up with an nLockTime of 0 rather than the current block (Andrew Stone)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrea Suisani
- Andrew Stone
- Justaphf
- Peter Tschipper


We also have backported changes from other projects, this is a list of PR ported from Core and Classic:

- `4a276240c` Cherry-pic from Classic: Make the QR code show a fork specific prefix
- `99cfe5237` Merge #10958: Update to latest Bitcoin patches for LevelDB
- `2da740663` Merge #10806: build: verify that the assembler can handle crc32 functions

Following all the indirect contributors whose work has been imported via the above backports:

- MarcoFalke
- Matt Corallo
- Jonas Schnelli
- Cory Fields
- Pieter Wuille
- TomZ
- Marko Falke
