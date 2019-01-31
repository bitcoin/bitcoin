This directory contains integration tests that test dashd and its
utilities in their entirety. It does not contain unit tests, which
can be found in [/src/test](/src/test), [/src/wallet/test](/src/wallet/test),
etc.

There are currently two sets of tests in this directory:

- [functional](/test/functional) which test the functionality of
dashd and dash-qt by interacting with them through the RPC and P2P
interfaces.
- [util](/test/util) which tests the dash utilities, currently only
dash-tx.

The util tests are run as part of `make check` target. The functional
tests are run by the travis continuous build process whenever a pull
request is opened. Both sets of tests can also be run locally.

# Running tests locally

Before tests can be run locally, Dash Core must be built.  See the [building instructions](/doc#building) for help.


### Functional tests

#### Dependencies

Many Dash specific tests require dash_hash. To install it:

- Clone the repo `git clone https://github.com/dashpay/dash_hash`
- Install dash_hash `cd dash_hash && python3 setup.py install`

The ZMQ functional test requires a python ZMQ library. To install it:

- on Unix, run `sudo apt-get install python3-zmq`
- on mac OS, run `pip3 install pyzmq`

#### Running the tests

Individual tests can be run by directly calling the test script, eg:

```
test/functional/wallet_hd.py
```

or can be run through the test_runner harness, eg:

```
test/functional/test_runner.py wallet_hd.py
```

You can run any combination (incl. duplicates) of tests by calling:

```
test/functional/test_runner.py <testname1> <testname2> <testname3> ...
```

Run the regression test suite with:

```
test/functional/test_runner.py
```

Run all possible tests with

```
test/functional/test_runner.py --extended
```

By default, up to 4 tests will be run in parallel by test_runner. To specify
how many jobs to run, append `--jobs=n`

The individual tests and the test_runner harness have many command-line
options. Run `test_runner.py -h` to see them all.

#### Troubleshooting and debugging test failures

##### Resource contention

The P2P and RPC ports used by the dashd nodes-under-test are chosen to make
conflicts with other processes unlikely. However, if there is another dashd
process running on the system (perhaps from a previous test which hasn't successfully
killed all its dashd nodes), then there may be a port conflict which will
cause the test to fail. It is recommended that you run the tests on a system
where no other dashd processes are running.

On linux, the test_framework will warn if there is another
dashd process running when the tests are started.

If there are zombie dashd processes after test failure, you can kill them
by running the following commands. **Note that these commands will kill all
dashd processes running on the system, so should not be used if any non-test
dashd processes are being run.**

```bash
killall dashd
```

or

```bash
pkill -9 dashd
```


##### Data directory cache

A pre-mined blockchain with 200 blocks is generated the first time a
functional test is run and is stored in test/cache. This speeds up
test startup times since new blockchains don't need to be generated for
each test. However, the cache may get into a bad state, in which case
tests will fail. If this happens, remove the cache directory (and make
sure dashd processes are stopped as above):

```bash
rm -rf cache
killall dashd
```

##### Test logging

The tests contain logging at different levels (debug, info, warning, etc). By
default:

- when run through the test_runner harness, *all* logs are written to
  `test_framework.log` and no logs are output to the console.
- when run directly, *all* logs are written to `test_framework.log` and INFO
  level and above are output to the console.
- when run on Travis, no logs are output to the console. However, if a test
  fails, the `test_framework.log` and dashd `debug.log`s will all be dumped
  to the console to help troubleshooting.

To change the level of logs output to the console, use the `-l` command line
argument.

`test_framework.log` and dashd `debug.log`s can be combined into a single
aggregate log by running the `combine_logs.py` script. The output can be plain
text, colorized text or html. For example:

```
combine_logs.py -c <test data directory> | less -r
```

will pipe the colorized logs from the test into less.

Use `--tracerpc` to trace out all the RPC calls and responses to the console. For
some tests (eg any that use `submitblock` to submit a full block over RPC),
this can result in a lot of screen output.

By default, the test data directory will be deleted after a successful run.
Use `--nocleanup` to leave the test data directory intact. The test data
directory is never deleted after a failed test.

##### Attaching a debugger

A python debugger can be attached to tests at any point. Just add the line:

```py
import pdb; pdb.set_trace()
```

anywhere in the test. You will then be able to inspect variables, as well as
call methods that interact with the dashd nodes-under-test.

If further introspection of the dashd instances themselves becomes
necessary, this can be accomplished by first setting a pdb breakpoint
at an appropriate location, running the test to that point, then using
`gdb` to attach to the process and debug.

For instance, to attach to `self.node[1]` during a run:

```bash
2017-06-27 14:13:56.686000 TestFramework (INFO): Initializing test directory /tmp/user/1000/testo9vsdjo3
```

use the directory path to get the pid from the pid file:

```bash
cat /tmp/user/1000/testo9vsdjo3/node1/regtest/dashd.pid
gdb /home/example/dashd <pid>
```

Note: gdb attach step may require ptrace_scope to be modified, or `sudo` preceding the `gdb`.
See this link for considerations: https://www.kernel.org/doc/Documentation/security/Yama.txt

### Util tests

Util tests can be run locally by running `test/util/bitcoin-util-test.py`.
Use the `-v` option for verbose output.

# Writing functional tests

You are encouraged to write functional tests for new or existing features.
Further information about the functional test framework and individual
tests is found in [test/functional](/test/functional).
