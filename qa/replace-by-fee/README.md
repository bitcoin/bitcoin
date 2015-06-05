Replace-by-fee regression test
==============================

First get version v0.4.0 of the python-bitcoinlib library. In this directory
run:

    git clone -n https://github.com/petertodd/python-bitcoinlib
    (cd python-bitcoinlib && git checkout f8606134765aa986537e897598df7b7833e0e6da)

Then run the tests themselves with a bitcoind available running in regtest
mode:

    ./replace-by-fee-tests.py
