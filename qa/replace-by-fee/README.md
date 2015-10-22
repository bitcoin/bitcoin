Replace-by-fee regression test
==============================

First get version v0.3.0 of the python-bitcoinlib library. In this directory
run:

    git clone -n https://github.com/petertodd/python-bitcoinlib
    (cd python-bitcoinlib && git checkout c481254c623cc9a002187dc23263cce3e05f5754)

Then run the tests themselves with a bitcoind available running in regtest
mode:

    ./replace-by-fee-tests.py
