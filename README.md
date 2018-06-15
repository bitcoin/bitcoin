Dobbscoin Core/SLACK integration/staging tree
=====================================
The Official CryptoCurrency of the Church of the SubGenius. http://www.subgenius.com

The ONLY crypto-currency accepted on the Pleasure Saucers. https://www.dobbscoin.info

Maybe you've come into a large inheritance, or your income just suddenly popped.
Maybe you gave birth to quintuplets or been recently divorced. Or maybe you just
feel uneasy about your money; where it's going or how far it will take you in the
future. Whatever your problem is, Dobbscoin can help you.

Will you join TheConspiracy’s mindless atheistic unknowing servitude to the Elder Bankers of the Universe and their MINIONS in some hideous World Government, or will you GET SLACK and FIGHT FOR FREEDOM as a zeal-crazed Priest-Warrior for ODIN?

Give yourself to SLACK freely, joyously, without an atom of restraint --NOW -- and your worries are over.

Get in on the ground floor of this lucrative alt., while difficulty is low.

Eternal Salvation or Triple your Money Back!!

What is Dobbscoin?
----------------

Dobbscoin is an excrimental new digital currency that enables the sending of
instant Slack to anyone, anywhere in the omniverse. Dobbscoin uses peer-to-peer
technology to operate with no central authority: managing transactions and
issuing Slack is carried out collectively by the network.

For more information, as well as an immediately useable binary version of
the Dobbscoin Core software, see https://github.com/dobbscoin/dobbscoin-source/releases

License
-------

Bitcoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/dobbscoin/dobbscoin-source/tags) are created
regularly to indicate new official, stable release versions of Bitcoin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

The developer [forum](http://dobbscoin.info/smf)
should be used to discuss complicated or controversial changes before working
on a patch set.

Developer IRC can be found on Freenode at #dobbscoin-dev

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

The Travis CI system makes sure that every pull request is built for Windows, Linux, and OS X, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/projects/p/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also subscribe to the [mailing list](https://groups.google.com/forum/#!forum/bitcoin-translators).
