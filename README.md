Chaincoin Core staging tree 0.16.99
===============================

master is the working branch, [for releases follow here](https://github.com/chaincoin/chaincoin/releases)

https://www.chaincoin.org


What is Chaincoin?
----------------

Chaincoin is an experimental new digital currency that enables anonymous, instant
payments to anyone, anywhere in the world. Chaincoin uses peer-to-peer technology
to operate with no central authority: managing transactions and issuing money
are carried out collectively by the network. Chaincoin Core is the name of the open
source software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Chaincoin Core software, see https://www.chaincoin.org/get-chaincoin/.


License
-------

Chaincoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be completely stable. [Tags](https://github.com/chaincoin/chaincoin/tags) are created
regularly to indicate new official, stable release versions of Chaincoin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](/doc/unit-tests.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/qa) are installed) with: `qa/pull-tester/rpc-tests.py`


The Travis CI system makes sure that every pull request is built for Windows, Linux, and macOS, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Chaincoin Core's Transifex page](https://www.transifex.com/projects/p/chaincoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also follow the translation channel in our [Discord](https://discord.gg/NabdcJ7).
