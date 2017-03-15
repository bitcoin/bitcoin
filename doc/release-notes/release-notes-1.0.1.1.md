Release Notes for Bitcoin Unlimited v1.0.1.1
==========================================

Bitcoin Unlimited version 1.0.1.1 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is an hotfix release.

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

Changes are as follows:

- Fix unwanted assertion Sending an invalid GET_XTHIN is a serious misbehavior and any node doing so will be DOS100 banned immediately.  Also sending a GET_XTHIN with an invalid message type will also cause the sendder to be banned. This bug cause the node to crash.
- Fix pruning when syncing a chain for the first time. iWhen  syncing a chain with pruning enabled there are at times new blocks arriving which make the nLastHeight equal to the tip of the blockchain, however this prevents block files from being removed during pruning.  By not downloading new blocks until the chain no longer in IsInitialDownload() the issue is prevented.

Commit details
--------------
- `95c594a` version 1.0.1.1 (Andrew Stone)
- `eee6a2d` Fix potential unwanted assertion (Peter Tschipper)
- `db93b0c` Merge pull request #291 from ptschip/dev_prune (gandrewstone)
- `b200b0d` Bump client version (Justaphf)
- `da4619d` Merge pull request #300 from Justaphf/dev_2017 (gandrewstone)
