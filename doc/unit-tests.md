Compiling/running unit tests
------------------------------------

Unit tests will be automatically compiled if dependencies were met in configure
and tests weren't explicitly disabled.

After configuring, they can be run with 'make check'.

To run the bitcoin core tests manually, launch src/test/test_bitcoin .

To add more bitcoin tests, add `BOOST_AUTO_TEST_CASE` functions to the existing
.cpp files in the test/ directory or add new .cpp files that
implement new BOOST_AUTO_TEST_SUITE sections.

To run the bitcoin-core-gui tests manually, launch src/qt/test/bitcoin-core-gui_test

To add more bitcoin-core-gui tests, add them to the `src/qt/test/` directory and
the `src/qt/test/test_main.cpp` file.
