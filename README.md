Bitcoin Classic integration/staging tree
=====================================

[![Build Status](https://travis-ci.org/bitcoinclassic/bitcoinclassic.svg?branch=master)](https://travis-ci.org/bitcoinclassic/bitcoinclassic)

https://bitcoinclassic.com

What is Bitcoin?
----------------

Bitcoin is an experimental new digital currency that enables instant payments to
anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network.

For more information, as well as an immediately useable, binary version of
the Bitcoin Classic software, see https://bitcoinclassic.com.

What is Bitcoin Classic?
------------------------

Bitcoin Classic is currently a one-time increase in total amount of transaction data permitted in a block from 1MB to 2MB, with limits on signature operations and hashing. We will have ports for master and 0.11.2, so that miners and businesses can upgrade to 2 MB blocks from any recent bitcoin software version they run.

Read the [block size increase BIP](https://github.com/gavinandresen/bips/blob/92e1efd0493c1cbde47304c9711f13f413cc9099/bip-bump2mb.mediawiki) for more information.

In the future Bitcoin Classic will continue to release updates that are in line with Satoshi’s whitepaper & vision, and are agreed upon by the community.

License
-------

Bitcoin Classic is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoinclassic/bitcoinclassic/tags) are created
regularly to indicate new official, stable release versions of Bitcoin Classic.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Complicated or controversial changes should be discussed within the communtiy before working on a patch set.

Community
---------

- Primary Website: https://bitcoinclassic.com/
- Slack: http://invite.bitcoinclassic.com/
- Reddit: https://www.reddit.com/r/Bitcoin_Classic/
- GitHub: https://github.com/bitcoinclassic
- ConsiderIt (issue voting): https://bitcoinclassic.consider.it/

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
These tests can be run with: `qa/pull-tester/rpc-tests.py`

The Travis CI system makes sure that every pull request is built for Windows
and Linux, OSX, and that unit and sanity tests are automatically run.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Classic's Transifex page](https://www.transifex.com/bitcoinclassic/bitcoinclassic/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.
