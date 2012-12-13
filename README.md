Bitcoin integration/staging tree
================================

http://www.bitcoin.org

Copyright (c) 2009-2012 Bitcoin Developers

What is Bitcoin?
----------------

Bitcoin is an experimental new digital currency that enables instant payments to
anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Bitcoin is also the name of the open source
software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Bitcoin client sofware, see http://www.bitcoin.org.

License
-------

Bitcoin is released under the terms of the MIT license. See `COPYING` for more
information or see http://opensource.org/licenses/MIT.

Development process
-------------------

Developers work in their own trees, then submit pull requests when they think
their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the Bitcoin
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
regularly to indicate new official, stable release versions of Bitcoin. If you
would like to help test the Bitcoin core, please contact <QA@BitcoinTesting.org>.

Feature branches are created when there are major new features being implemented
by several people.

From time to time, a pull request may become outdated. If this occurs, and the
pull request is no longer automatically mergeable, a comment warning of closure
will be posted on the pull request. The pull request will be closed 15 days
after the warning if the author of the pull request does not rebase or otherwise
update the pull request code to be automatically mergeable. A pull request
closed in this manner will have its corresponding issue labeled 'stagnant'.

Issues with no commits will be given a similar warning, and closed after 15 days
from their last activity. Issues closed in this manner will be labeled 'stale'.

Requests to reopen closed pull requests and/or issues can be submitted to 
<QA@BitcoinTesting.org>.
