# Unit tests

The sources in this directory are unit test cases. Boost includes a
unit testing framework, and since Bitcoin Core already uses Boost, it makes
sense to simply use this framework rather than require developers to
configure some other framework (we want as few impediments to creating
unit tests as possible).

The build system is set up to compile an executable called `test_bitcoin`
that runs all of the unit tests. The main source file for the test library is found in
`util/setup_common.cpp`.

All steps are to be run from your terminal emulator, i.e. the command line.

### Compiling/running unit tests

Unit tests will be automatically compiled if dependencies were met in `./configure`
and tests weren't explicitly disabled.

1. Ensure the dependencies are installed. Note that `ccache` at the end of the list isn't strictly required, but you'll probably want to install it (see below).
     * Linux: ```sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils python3 libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler git libsqlite3-dev ccache```
     * macOS: ```(with command line tools and Homebrew already installed): brew install automake berkeley-db4 libtool boost miniupnpc pkg-config python qt libevent qrencode sqlite ccache```

2. Download the Bitcoin source files by git cloning the repository:
    ```git clone https://github.com/bitcoin/bitcoin.git```

3. Install Berkeley DB (BDB) v4.8, a backward-compatible version needed for the wallet, using the installation script included in the Bitcoin Core contrib directory. If you have another version of BDB already installed that you wish to use, skip this section and add ```--with-incompatible-bdb``` to your ```./configure``` options below instead.

    * Enter your local copy of the bitcoin repository: ```cd bitcoin```
    * Now that you are in the root of the bitcoin repository, run: ``` ./contrib/install_db4.sh `pwd` ```
    * Take note of the instructions displayed in the terminal at the end of the BDB installation process: 
        ```db4 build complete.
           When compiling bitcoind, run `./configure` in the following way:
           export BDB_PREFIX='<PATH-TO>/db4'
           ./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include" ...
       ```
4. Compile from a tagged release branch, unless you wish to test a specific branch or PR.
    * See tags and descriptions ordered by most recent last: ```git tag -n | sort -V```
    * To switch to a specific tagged release: ```git checkout <TAG>``` (for ex: ```git checkout v0.21.0```)

5. Compile/build Bitcoin from source:
    * ``` export BDB_PREFIX='<PATH-TO>/db4' ``` (for ex: ``` export BDB_PREFIX='/home/pi/bitcoin/db4' ```
    * ```./autogen.sh```
    * If using BDB4.8 (backwards-compatible version of BerkeleyDB used for wallets, recommended): ```./configure BDB_LIBS="-L${BDB_PREFIX}/lib -ldb_cxx-4.8" BDB_CFLAGS="-I${BDB_PREFIX}/include"```, otherwise: ```./configure --with-incompatible-bdb```
    * Compilation using ```make```
        * Use multiple CPU cores to compile to significantly reduce times (recommended): 
            * Linux: ```make -j "$(($(nproc)+1))"```
            * macOS: ```make -j "$(($(sysctl -n hw.physicalcpu)+1))"```
        * Single core: ```make```

To run the unit tests manually, launch `src/test/test_bitcoin`. To recompile
after a test file was modified, run `make` and then run the test again. If you
modify a non-test file, use `make -C src/test` to recompile only what's needed
to run the unit tests.

To add more unit tests, add `BOOST_AUTO_TEST_CASE` functions to the existing
.cpp files in the `test/` directory or add new .cpp files that
implement new `BOOST_AUTO_TEST_SUITE` sections.

To run the GUI unit tests manually, launch `src/qt/test/test_bitcoin-qt`

To add more GUI unit tests, add them to the `src/qt/test/` directory and
the `src/qt/test/test_main.cpp` file.

### Running individual tests

`test_bitcoin` has some built-in command-line arguments; for
example, to run just the `getarg_tests` verbosely:

    test_bitcoin --log_level=all --run_test=getarg_tests -- DEBUG_LOG_OUT

`log_level` controls the verbosity of the test framework, which logs when a
test case is entered, for example. The `DEBUG_LOG_OUT` after the two dashes
redirects the debug log, which would normally go to a file in the test datadir
(`BasicTestingSetup::m_path_root`), to the standard terminal output.

... or to run just the doubledash test:

    test_bitcoin --run_test=getarg_tests/doubledash

Run `test_bitcoin --help` for the full list.

### Adding test cases

To add a new unit test file to our test suite you need
to add the file to `src/Makefile.test.include`. The pattern is to create
one test file for each class or source file for which you want to create
unit tests. The file naming convention is `<source_filename>_tests.cpp`
and such files should wrap their tests in a test suite
called `<source_filename>_tests`. For an example of this pattern,
see `uint256_tests.cpp`.

### Logging and debugging in unit tests

`make check` will write to a log file `foo_tests.cpp.log` and display this file
on failure. For running individual tests verbosely, refer to the section
[above](#running-individual-tests).

To write to logs from unit tests you need to use specific message methods
provided by Boost. The simplest is `BOOST_TEST_MESSAGE`.

For debugging you can launch the `test_bitcoin` executable with `gdb`or `lldb` and
start debugging, just like you would with any other program:

```bash
gdb src/test/test_bitcoin
```

#### Segmentation faults

If you hit a segmentation fault during a test run, you can diagnose where the fault
is happening by running `gdb ./src/test/test_bitcoin` and then using the `bt` command
within gdb.

Another tool that can be used to resolve segmentation faults is
[valgrind](https://valgrind.org/).

If for whatever reason you want to produce a core dump file for this fault, you can do
that as well. By default, the boost test runner will intercept system errors and not
produce a core file. To bypass this, add `--catch_system_errors=no` to the
`test_bitcoin` arguments and ensure that your ulimits are set properly (e.g. `ulimit -c
unlimited`).

Running the tests and hitting a segmentation fault should now produce a file called `core`
(on Linux platforms, the file name will likely depend on the contents of
`/proc/sys/kernel/core_pattern`).

You can then explore the core dump using
``` bash
gdb src/test/test_bitcoin core

(gbd) bt  # produce a backtrace for where a segfault occurred
```

#### Additional guide
Below is a great guide on how to run the tests if you're not able to compile the tests successfully yet:
https://jonatack.github.io/articles/how-to-compile-bitcoin-core-and-run-the-tests
