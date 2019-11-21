# BitGreen Core integration/staging tree

[![Build Status](https://travis-ci.org/bitgreen/bitgreen.svg?branch=master)](https://travis-ci.org/bitgreen/bitgreen) ![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)

## What is BitGreen?

BitGreen is a sustainable cryptocurrency modeled after Satoshi Nakamoto’s vision for Bitcoin. It is a decentralized, peer-to-peer transactional currency designed to offer a solution to the problem posed by the exponential increase in energy consumed by Bitcoin and other proof-of-work currencies.
Proof-of-work mining is environmentally unsustainable due to the electricity used by high-powered mining hardware. BitGreen utilizes the Green Protocol, an energy efficient proof-of-stake algorithm inspired by peercoin, can be mined on any computer, and will never require specialized mining equipment. The Green Protocol offers a simple solution to Bitcoin sustainability issues and provides a faster, more scalable blockchain that is better suited for daily transactional use.

More information at [bitg.org](http://www.bitg.org)

Please reach out at info@bitg.org

## License

BitGreen Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

## Development Process

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitgreen/bitgreen/tags) are created
regularly to indicate new official, stable release versions of BitGreen Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

## Testing

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The Travis CI system makes sure that every pull request is built for Windows, Linux, and macOS, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
