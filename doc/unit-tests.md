About Bitcoin unit tests
====================================

The Bitcoin software currently includes couple of types of unit tests:

- Boost Test based unit tests for the non-GUI parts of the C/C++ code
- QtTest based unit tests for the GUI code
- doctests for some Python modules (currently very limited)

This document currently focuses on the C++ unit tests, but there is some
info on Python doctests at the end.


Compiling C++ unit tests
------------------------------------

C++ Unit tests will be automatically compiled if dependencies were met
in `./configure` and tests were not explicitly disabled.



Running unit tests sequentially
------------------------------------

After configuring, the C++ unit tests can be run with `make check`.
This runs tests of several libraries.

To run the bitcoin unit test suite sequentially, launch
`src/test/test_bitcoin`.

You can run it with the `--help` option to get a list of parameters
supported by the Boost Test framework.  Some of these are very useful,
specifically '--run_test=<name>' to run a particular subset of tests, e.g.

    $ src/test/test_bitcoin --run_test=transaction_tests

If on Boost 1.59 or higher, you can also get a full list of tests
using `--list_content`:

    $ src/test/test_bitcoin --list_content

The bitcoin-qt GUI tests use a different framework which does not support
the above options.
You can run these tests manually by launching `src/qt/test/test_bitcoin-qt`.


Running Boost unit tests in parallel
---------------------------------------

There is a wrapper tool in contrib/testtools/gtest-parallel-bitcoin
which can execute the Boost-based C++ unit tests in parallel

NOTE: only works if Bitcoin has been compiled against Boost 1.59 or later,
as it uses some command line argument features only provided by Boost Test
from that version onwards.

Assuming you have contrib/testtools/ in your path, it can be run by passing
it the name of the test executable, e.g.

    $ gtest-parallel-bitcoin src/test/test_bitcoin

It works by extracting the list of tests contained in the executable
using the '--list_content' command line parameter, and then running the
tests individually in parallel.

Running with `--help` provides help about the command line options.
Using `--format=raw` option provides detailed worker thread output during
execution.
The `--print_test_times` will print a detailed breakdown of all test times.

The wrapper creates and maintains a score database of test execution times
in ~/.gtest-parallel-bitcoin-times .
When it executes tests in repeated runs, it does so in order from longest
to shortest test.

You can also specify the `--failed` option to run only previously failed
and new tests.
The `--timeout` option can be used to constrain runtime.
Interrupted tests will be listed.
A `-r` option can be used to repeat individual tests multiple times during
a run.

If you have enough free CPU cores, this script can reduce the runtime of
the C++ unit test suite to approximately that of the slowest test.


Adding more C++ unit tests
------------------------------------

To add more C++ tests, add `BOOST_AUTO_TEST_CASE` functions to the existing
.cpp files in the `test/` directory or add new .cpp files that
implement new BOOST_AUTO_TEST_SUITE sections.


To add more bitcoin-qt tests, add them to the `src/qt/test/` directory and
the `src/qt/test/test_main.cpp` file.


Running Python module doctests
------------------------------------

A Python module which supports doctests usually contains code like this
for when it is invoked as a standalone program:

    if __name__ == "__main__":
        import doctest
        doctest.testmod()

Otherwise, check for presence of import of the 'doctest' in general.

Doctests can be executed by running the module by itself through an
interpreter as if it were a standalone program, e.g.

    $ python3 qa/pull-tester/test_classes.py

If there is a failure in tests, it will be reported, otherwise the
command should return silently with exit code 0.


Adding Python doctests
------------------------------------

There is lots of good documentation about this out there.
Good places to start:

https://docs.python.org/3/library/doctest.html
https://en.wikipedia.org/wiki/Doctest

