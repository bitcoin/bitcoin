Regression tests of RPC interface
=================================

Bash scripts that use the RPC interface and command-line bitcoin-cli to test
full functionality in -regtest mode.

wallet.sh : Exercise wallet send/receive code.

txnmall.sh : Test proper accounting of malleable transactions

conflictedbalance.sh : More testing of malleable transaction handling

util.sh : useful re-usable bash functions 
