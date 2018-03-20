Syscoin integration/staging tree
================================

[![Join the chat at https://gitter.im/syscoin/syscoin](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/syscoin/syscoin?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

https://www.syscoin.org

Copyright (c) 2009-2014 Bitcoin Developers
Copyright (c) 2013-2018 Syscoin Developers

What is Syscoin?
----------------

Syscoin is a merge-minable SHA256 coin which provides an array of useful services
which leverage the bitcoin protocol and blockchain technology.

 - 1 minute block targets, diff retarget each block using KGW(7/98) 
 - Flexible rewards schedule paying 25% to miners and 75% to masternodes
 - 888 million total coins
 - SHA256 Proof of Work
 - Fast-response KGW difficulty adjustment algorithm
 - Merge mineable with any PoW coin
 - Minable either exclusively or via merge-mining 
 - Network service fees burned

Services include:

- Decentralized Identity reservation, ownership & exchange
- Digital certificate storage, ownership & exchange
- Distributed marketplate & exchange
- Digital Services Provider marketplace & platform
- Digital Asset Creation and Management
- Decentralized Escrow service

For more information, as well as an immediately useable, binary version of
the Syscoin client sofware, see https://www.syscoin.org.

License
-------

Syscoin is released under the terms of the MIT license. See `COPYING` for more
information or see http://opensource.org/licenses/MIT.

Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Syscoin
development team members simply pulls it.

If it is a *more complicated or potentially controversial* change, then the patch
submitter will be asked to start a discussion (if they haven't already) on the
[mailing list](http://sourceforge.net/mailarchive/forum.php?forum_name=bitcoin-development).

The patch will be accepted if there is broad consensus that it is a good thing.
Developers should expect to rework and resubmit patches if the code doesn't
match the project's coding conventions (see `doc/coding.txt`) or are
controversial.

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/bitcoin/bitcoin/tags) are created
regularly to indicate new official, stable release versions of Syscoin.

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test. Please be patient and help out, and
remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write unit tests for new code, and to
submit new unit tests for old code.

Unit tests for the core code are in `src/test/`. To compile and run them:

    make; cd src/test; ./test_syscoin;

