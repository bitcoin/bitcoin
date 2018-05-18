Syscoin Core staging tree 
=========================

`master:` [![Build Status](https://travis-ci.org/syscoin/syscoin.svg?branch=master)](https://travis-ci.org/syscoin/syscoin) 

http://www.syscoin.org

What is Syscoin?
----------------

Syscoin is a merge-minable SHA256 coin which provides an array of useful services
which leverage the bitcoin protocol and blockchain technology.

- 1 minute block targets, diff retarget each block using Dark Gravity Wave(24) 
- 888 million total coins (deflation 5 percent per year, deflation on all payouts)
- Block time: 60 seconds target
- Rewards: 38.5 Syscoins per block deflated 5 percent per year. 
  - 10 percent to governance proposals (3.85 Syscoins / block). 
  - 22.5 percent to miners (8.6625 Syscoins / block)
  - 67.5 percent to masternodes (25.9875 Syscoins / block)
- SHA256 Proof of Work
- Mineable either exclusively or via merge-mining any SHA256 PoW coin
- Syscoin data service fees burned
- Masternode collateral requirement: 100000 Syscoins
- Masternode seniority: 3 percent every 4 months until 27 percent over 3 years
- Governance proposals payout schedule: every month
- Governance funding per round (168630 Syscoins per month)

Services include:

- Hybrid layer 2 PoW/PoS consensus with bonded validator system (masternodes)
- Decentralized governance (blockchain pays for work via proposals and masternode votes)
- Decentralized identity reservation, ownership, and exchange
- Digital certificate storage, ownership, and exchange
- Distributed marketplace and exchange
- Digital services provider marketplace and platform
- Digital asset creation and management
- Decentralized escrow service

For more information, as well as an immediately useable, binary version of
the Syscoin client sofware, see https://www.syscoin.org.


License
-------

Syscoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is meant to be stable. Development is normally done in separate branches.
[Tags](https://github.com/syscoin/syscoin/tags) are created to indicate new official,
stable release versions of Syscoin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](/doc/unit-tests.md) for new code, and to
submit new unit tests for old code.

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/qa) are installed) with: `qa/pull-tester/rpc-tests.py`

Syscoin test suites can run by `cd src/test && ./test_syscoin`

The Travis CI system makes sure that every pull request is built for Windows
and Linux, OS X, and that unit and sanity tests are automatically run.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Syscoin Core's Transifex page](https://www.transifex.com/projects/p/syscoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

