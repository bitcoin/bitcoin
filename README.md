Qitcoin Core integration/staging tree
=======================================

https://qiantongchain.com

The Crypto Currency System Based on CPoC.

Pay Tribute to Cryptocurrency Pioneers [Bitcoin](https://bitcoincore.org)
and [Burst](https://www.burst-coin.org)!

About Qitcoin
---------------

- Qitcoin is a new type of crypto currency based on Proof of Capacity.
- Qitcoin uses an upgraded version of cPOC mining (Conditioned Proof of Capacity), with a perfect economic model and consensus algorithm.
- Qitcoin uses hard disk as the participant of consensus, which reduces power consumption.
- Qitcoin mining lowers the entry barriers, and makes the coin generation process more decentralized, secure and reliable.
- Compared with POW mining, cPOC mining saves energy, consumes much less power, has lower noise, no heat, and is anti-ASIC. cPOC-mining-based Qitcoin can realize the original intention of Satoshi Nakamoto ---- everyone can become a miner.
- Lowered the cost of credit, increased the strength and breadth of consensus, and improved the security of the consensus architecture

For more information, as well as an immediately useable, binary version of
the Qitcoin Core software, see https://qiantongchain.com/#wallet, or read the
[original whitepaper](https://qiantongchain.com/QTC-Whitepaper2.0.pdf).

License
-------

Qitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/qitcoin/qitcoin/tags) are created
regularly to indicate new official, stable release versions of Qitcoin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

Testing
-------

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

Translations
------------

The Qitcoin Core base on Bitcoin Core and shares resources with it.

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also subscribe to the [mailing list](https://groups.google.com/forum/#!forum/bitcoin-translators).
