Regression tests of RPC interface
=================================

wallet.sh : Exercise wallet send/receive code.

walletbackup.sh : Exercise wallet backup / dump / import

txnmall.sh : Test proper accounting of malleable transactions

conflictedbalance.sh : More testing of malleable transaction handling

util.sh : useful re-usable bash functions


Tips for creating new tests
===========================

To cleanup after a failed or interrupted test:
  killall bitcoind
  rm -rf test.*

The most difficult part of writing reproducible tests is
keeping multiple nodes in sync. See WaitBlocks,
WaitPeers, and WaitMemPools for how other tests
deal with this.
