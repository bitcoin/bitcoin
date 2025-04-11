# Contributing to libsecp256k1

## Scope

libsecp256k1 is a library for elliptic curve cryptography on the curve secp256k1, not a general-purpose cryptography library.
The library primarily serves the needs of the Bitcoin Core project but provides additional functionality for the benefit of the wider Bitcoin ecosystem.

## Adding new functionality or modules

The libsecp256k1 project welcomes contributions in the form of new functionality or modules, provided they are within the project's scope.

It is the responsibility of the contributors to convince the maintainers that the proposed functionality is within the project's scope, high-quality and maintainable.
Contributors are recommended to provide the following in addition to the new code:

* **Specification:**
    A specification can help significantly in reviewing the new code as it provides documentation and context.
    It may justify various design decisions, give a motivation and outline security goals.
    If the specification contains pseudocode, a reference implementation or test vectors, these can be used to compare with the proposed libsecp256k1 code.
* **Security Arguments:**
    In addition to a defining the security goals, it should be argued that the new functionality meets these goals.
    Depending on the nature of the new functionality, a wide range of security arguments are acceptable, ranging from being "obviously secure" to rigorous proofs of security.
* **Relevance Arguments:**
    The relevance of the new functionality for the Bitcoin ecosystem should be argued by outlining clear use cases.

These are not the only factors taken into account when considering to add new functionality.
The proposed new libsecp256k1 code must be of high quality, including API documentation and tests, as well as featuring a misuse-resistant API design.

We recommend reaching out to other contributors (see [Communication Channels](#communication-channels)) and get feedback before implementing new functionality.

## Communication channels

Most communication about libsecp256k1 occurs on the GitHub repository: in issues, pull request or on the discussion board.

Additionally, there is an IRC channel dedicated to libsecp256k1, with biweekly meetings (see channel topic).
The channel is `#secp256k1` on Libera Chat.
The easiest way to participate on IRC is with the web client, [web.libera.chat](https://web.libera.chat/#secp256k1).
Chat history logs can be found at https://gnusha.org/secp256k1/.

## Contributor workflow & peer review

The Contributor Workflow & Peer Review in libsecp256k1 are similar to Bitcoin Core's workflow and review processes described in its [CONTRIBUTING.md](https://github.com/bitcoin/bitcoin/blob/master/CONTRIBUTING.md).

### Coding conventions

In addition, libsecp256k1 tries to maintain the following coding conventions:

* No runtime heap allocation (e.g., no `malloc`) unless explicitly requested by the caller (via `secp256k1_context_create` or `secp256k1_scratch_space_create`, for example). Moreover, it should be possible to use the library without any heap allocations.
* The tests should cover all lines and branches of the library (see [Test coverage](#coverage)).
* Operations involving secret data should be tested for being constant time with respect to the secrets (see [src/ctime_tests.c](src/ctime_tests.c)).
* Local variables containing secret data should be cleared explicitly to try to delete secrets from memory.
* Use `secp256k1_memcmp_var` instead of `memcmp` (see [#823](https://github.com/bitcoin-core/secp256k1/issues/823)).
* As a rule of thumb, the default values for configuration options should target standard desktop machines and align with Bitcoin Core's defaults, and the tests should mostly exercise the default configuration (see [#1549](https://github.com/bitcoin-core/secp256k1/issues/1549#issuecomment-2200559257)).

#### Style conventions

* Commits should be atomic and diffs should be easy to read. For this reason, do not mix any formatting fixes or code moves with actual code changes. Make sure each individual commit is hygienic: that it builds successfully on its own without warnings, errors, regressions, or test failures.
* New code should adhere to the style of existing, in particular surrounding, code. Other than that, we do not enforce strict rules for code formatting.
* The code conforms to C89. Most notably, that means that only `/* ... */` comments are allowed (no `//` line comments). Moreover, any declarations in a `{ ... }` block (e.g., a function) must appear at the beginning of the block before any statements. When you would like to declare a variable in the middle of a block, you can open a new block:
    ```C
    void secp256k_foo(void) {
        unsigned int x;              /* declaration */
        int y = 2*x;                 /* declaration */
        x = 17;                      /* statement */
        {
            int a, b;                /* declaration */
            a = x + y;               /* statement */
            secp256k_bar(x, &b);     /* statement */
        }
    }
    ```
* Use `unsigned int` instead of just `unsigned`.
* Use `void *ptr` instead of `void* ptr`.
* Arguments of the publicly-facing API must have a specific order defined in [include/secp256k1.h](include/secp256k1.h).
* User-facing comment lines in headers should be limited to 80 chars if possible.
* All identifiers in file scope should start with `secp256k1_`.
* Avoid trailing whitespace.
* Use the constants `EXIT_SUCCESS`/`EXIT_FAILURE` (defined in `stdlib.h`) to indicate program execution status for examples and other binaries.

### Tests

#### Coverage

This library aims to have full coverage of reachable lines and branches.

To create a test coverage report, configure with `--enable-coverage` (use of GCC is necessary):

    $ ./configure --enable-coverage

Run the tests:

    $ make check

To create a report, `gcovr` is recommended, as it includes branch coverage reporting:

    $ gcovr --exclude 'src/bench*' --print-summary

To create a HTML report with coloured and annotated source code:

    $ mkdir -p coverage
    $ gcovr --exclude 'src/bench*' --html --html-details -o coverage/coverage.html

#### Exhaustive tests

There are tests of several functions in which a small group replaces secp256k1.
These tests are *exhaustive* since they provide all elements and scalars of the small group as input arguments (see [src/tests_exhaustive.c](src/tests_exhaustive.c)).

### Benchmarks

See `src/bench*.c` for examples of benchmarks.
