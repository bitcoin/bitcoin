# RapidCheck property-based testing for Bitcoin Core

## Concept

Property-based testing is experimentally being added to Bitcoin Core with
[RapidCheck](https://github.com/emil-e/rapidcheck), a C++ framework for
property-based testing inspired by the Haskell library
[QuickCheck](https://hackage.haskell.org/package/QuickCheck).

RapidCheck performs random testing of program properties. A specification of the
program is given in the form of properties which functions should satisfy, and
RapidCheck tests that the properties hold in a large number of randomly
generated cases.

If an exception is found, RapidCheck tries to find the smallest case, for some
definition of smallest, for which the property is still false and displays it as
a counter-example. For example, if the input is an integer, RapidCheck tries to
find the smallest integer for which the property is false.

## Running

If RapidCheck is installed, Bitcoin Core will automatically run the
property-based tests with the unit tests during `make check`, unless the
`--without-rapidcheck` flag is passed when configuring.

For more information, run `./configure --help` and see `--with-rapidcheck` under
Optional Packages.

## Setup

The following instructions have been tested with Linux Debian and macOS.

1. Clone the RapidCheck source code and cd into the repository.

    ```shell
    git clone https://github.com/emil-e/rapidcheck.git
    cd rapidcheck
    ```

2. Build RapidCheck (requires CMake to be installed).

    ```shell
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true -DRC_ENABLE_BOOST_TEST=ON $(pwd)
    make && make install
    ```

3. Configure Bitcoin Core with RapidCheck.

    `cd` to the directory of your local bitcoin repository and run
    `./configure`. In the output you should see:

    ```shell
    checking rapidcheck.h usability... yes
    checking rapidcheck.h presence... yes
    checking for rapidcheck.h... yes
    [...]
    Options used to compile and link:
    [...]
      with test     = yes
        with prop   = yes
    ```

4. Build Bitcoin Core with RapidCheck.

    Now you can run `make` and should see the property-based tests compiled with
    the unit tests:

    ```shell
    Making all in src
    [...]
    CXX      test/gen/test_bitcoin-crypto_gen.o
    CXX      test/test_bitcoin-key_properties.o
    ```

5. Run the unit tests with `make check`. The property-based tests will be run
   with the unit tests.

    ```shell
    Running tests: crypto_tests from test/crypto_tests.cpp
    [...]
    Running tests: key_properties from test/key_properties.cpp
    ```

That's it! You are now running property-based tests in Bitcoin Core.
