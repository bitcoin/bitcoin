Replace-by-fee regression tests
===============================

First get version v0.3.0 of the python-bitcoinlib library. In this directory
run:

    git clone -n https://github.com/petertodd/python-bitcoinlib
    (cd python-bitcoinlib && git checkout c481254c623cc9a002187dc23263cce3e05f5754)

Then run the tests themselves with a bitcoind available running in regtest
mode. There's separate tests for full and first-seen-safe RBF. The former:

    ./full-rbf-tests.py

To run the latter you'll need to restart bitcoind using the
fullrbfactivationtime argument to disable full-RBF on regtest:

    bitcoind -fullrbfactivationtime=0 -regtest

Followed by:

    ./fss-rbf-tests.py
