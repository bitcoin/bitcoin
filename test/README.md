This directory contains integration tests that test bitcoind and its
utilities in their entirety. It does not contain unit tests, which
can be found in [/src/test](/src/test), [/src/wallet/test](/src/wallet/test),
etc.

There are currently two sets of tests in this directory:

- [functional](/test/functional) which test the functionality of 
bitcoind and bitcoin-qt by interacting with them through the RPC and P2P
interfaces.
- [util](test/util) which tests the bitcoin utilities, currently only
bitcoin-tx.

The util tests are run as part of `make check` target. The functional
tests are run by the travis continuous build process whenever a pull
request is opened. Both sets of tests can also be run locally.

Functional Test dependencies
============================
The ZMQ functional test requires a python ZMQ library. To install it:

- on Unix, run `sudo apt-get install python3-zmq`
- on mac OS, run `pip3 install pyzmq`

Running tests locally
=====================

Build for your system first. Be sure to enable wallet, utils and daemon when you configure. Tests will not run otherwise.

Functional tests
----------------

You can run any single test by calling

    test/functional/test_runner.py <testname>

Or you can run any combination of tests by calling

    test/functional/test_runner.py <testname1> <testname2> <testname3> ...

Run the regression test suite with

    test/functional/test_runner.py

Run all possible tests with

    test/functional/test_runner.py --extended

By default, tests will be run in parallel. To specify how many jobs to run,
append `--jobs=n` (default n=4).

If you want to create a basic coverage report for the RPC test suite, append `--coverage`.

Possible options, which apply to each individual test run:

```
  -h, --help            show this help message and exit
  --nocleanup           Leave bitcoinds and test.* datadir on exit or error
  --noshutdown          Don't stop bitcoinds after the test execution
  --srcdir=SRCDIR       Source directory containing bitcoind/bitcoin-cli
                        (default: ../../src)
  --tmpdir=TMPDIR       Root directory for datadirs
  --tracerpc            Print out all RPC calls as they are made
  --coveragedir=COVERAGEDIR
                        Write tested RPC commands into this directory
```

If you set the environment variable `PYTHON_DEBUG=1` you will get some debug
output (example: `PYTHON_DEBUG=1 test/functional/test_runner.py wallet`).

A 200-block -regtest blockchain and wallets for four nodes
is created the first time a regression test is run and
is stored in the cache/ directory. Each node has 25 mature
blocks (25*50=1250 BTC) in its wallet.

After the first run, the cache/ blockchain and wallets are
copied into a temporary directory and used as the initial
test state.

If you get into a bad state, you should be able
to recover with:

```bash
rm -rf cache
killall bitcoind
```

Util tests
----------

Util tests can be run locally by running `test/util/bitcoin-util-test.py`. 
Use the `-v` option for verbose output.

Writing functional tests
========================

You are encouraged to write functional tests for new or existing features.
Further information about the functional test framework and individual 
tests is found in [test/functional](/test/functional).
