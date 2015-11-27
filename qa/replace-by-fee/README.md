Replace-by-fee regression tests
===============================

First get version v0.5.0 of the python-bitcoinlib library. In this directory
run:

    git clone -n https://github.com/petertodd/python-bitcoinlib
    (cd python-bitcoinlib && git checkout 8270bfd9c6ac37907d75db3d8b9152d61c7255cd)

Then run the tests themselves with a bitcoind available running in regtest
mode:

    ./rbf-tests.py
