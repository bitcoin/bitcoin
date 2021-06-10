Bitcoin Core Integration/Staging Tree
=====================================

https://bitcoincore.org

For an immediately usable, binary version of the Bitcoin Core software, see
https://bitcoincore.org/en/download/.

What is Bitcoin?
----------------

Bitcoin is an experimental digital currency that enables instant payments to
anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Bitcoin Core is the name of open source
software which enables the use of this currency.

For more information, read the original Bitcoin whitepaper.

[License Information](COPYING) - MIT License Permissions/Limitations/Conditions

[Documents](/doc) - Bitcoin Core Documentation (see `doc/build-*.md` for instructions) 

[Tags](https://github.com/bitcoin/bitcoin/tags) - Newest stable release versions

[GUI](https://github.com/bitcoin-core/gui) - Separate repository exclusively for GUI development
(**Please don't fork unless for development reasons**)

[Contributing Workflow](CONTRIBUTING.md) - Process/guidelines for contributing 
(Newcomers are welcome)

[Developer Notes](doc/developer-notes.md) - Useful hints on development process Bitcoin Core integration/staging tree

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

[Unit Tests](/src/test/README.md) - Developers are strongly encouraged to write unit tests for new code and to submit new unit tests for old code. Unit tests can be compiled and run (assuming they weren't disabled in configure) with: `make check`. 

[Regression and Integration Tests](/test) are written in Python. Run with `test/functional/test_runner.py`

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

[Bitcoin Core's Transifex Page](https://www.transifex.com/bitcoin/bitcoin/) - Changes to translations as well as new translations can be submitted here

[Translation Process](doc/translation_process.md) - Documentation on how translations are pulled from Transifex and merged into git repository. (**Important**: We do not accept translation changes as pull requests because the next pull from Transifex would automatically overwrite them.)

