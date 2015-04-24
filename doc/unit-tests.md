Compiling/running unit tests
------------------------------------

Unit tests will be automatically compiled if dependencies were met in configure
and tests weren't explicitly disabled.

After configuring, they can be run with 'make check'.

To run the creditsd tests manually, launch src/test/test_credits .

To add more creditsd tests, add `BOOST_AUTO_TEST_CASE` functions to the existing
.cpp files in the test/ directory or add new .cpp files that
implement new BOOST_AUTO_TEST_SUITE sections.

To run the credits-qt tests manually, launch src/qt/test/credits-qt_test

To add more credits-qt tests, add them to the `src/qt/test/` directory and
the `src/qt/test/test_main.cpp` file.
