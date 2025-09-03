Bitcoin Knots
=============

https://bitcoinknots.org

For an immediately usable, binary version of the Bitcoin Knots software, see
the website.

What is Bitcoin Knots?
----------------------

Bitcoin Knots connects to the Bitcoin peer-to-peer network to download and fully
validate blocks and transactions. It also includes a wallet and graphical user
interface, which can be optionally built.

Further information about Bitcoin Knots is available in the [doc folder](/doc).

License
-------

Bitcoin Knots is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

Development generally takes place as part of [Bitcoin Core](https://github.com/bitcoin/bitcoin), and is merged into
Knots for each release.

Even if your pull request to Core is closed, or if your feature is not
suitable for Core (eg, because it builds on a feature not supported in Core;
relies on centralised services; etc), it may still be eligible for inclusion
in Bitcoin Knots. In this case, a pull request may be opened on the
[Knots GitHub](https://github.com/bitcoinknots/bitcoin) for review and consideration.
When accepted, you are expected to maintain the submitted branch in your own
repository, and it will be automatically merged into new releases of Knots.

Developer IRC can be found on Freenode at #bitcoin-dev.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled during the generation of the build system) with: `ctest`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python.
These tests can be run (if the [test dependencies](/test) are installed) with: `build/test/functional/test_runner.py`
(assuming `build` is your build directory).

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS,
and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://explore.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.
