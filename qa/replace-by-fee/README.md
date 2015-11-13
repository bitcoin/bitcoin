Replace-by-fee regression test
==============================

First get the python-bitcoinlib library. In this directory run:

    git clone -n https://github.com/petertodd/python-bitcoinlib
    (cd python-bitcoinlib && git checkout 69be70c17ed18a423352352ee5a514229ad5f9d7)

Then run the tests themselves with a bitcoind available running in regtest
mode:

    ./replace-by-fee-tests.py
