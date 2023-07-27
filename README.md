🚀 Welcome to Bitcoin Core 🎉
=====================================

https://bitcoincore.org

For an immediately usable, binary version of the Bitcoin Core software, see
https://bitcoincore.org/en/download/.

What is Bitcoin Core? 🤔
---------------------

Bitcoin Core is your gateway to the Bitcoin network. It downloads and validates transactions and blocks in real-time! It also includes a wallet and graphical user interface (GUI) which can optionally be built.

Curious to know more? Find further details in [doc folder](/doc)

License 📃
-------

Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process 💡
-------------------

Our `master` branch is regularly built (see `doc/build-*.md` for instructions) and tested, although it might not always be stable. Official stable release versions are marked with [tags](https://github.com/bitcoin/bitcoin/tags).

We use the https://github.com/bitcoin-core/gui repository solely for GUI development. Its master branch serves as a clone in all monotree repositories. It doesn't have release branches and tags there, so you only need to fork it for development purposes.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

Testing 🧪
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing 🤖

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python 🐍.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The CI (Continuous Integration) systems make sure that every pull request is built for Windows, Linux, and macOS,
and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing ✅

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations 🌎
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: Please note, we do not accept changes to translations as GitHub pull requests, as the next Transifex pull would overwrite them.
