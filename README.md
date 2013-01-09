
Bitcoin integration/staging tree

Development process
===================

Developers work in their own trees, then submit pull requests when
they think their feature or bug fix is ready.

If it is a simple/trivial/non-controversial change, then one of the
bitcoin development team members simply pulls it.

If it is a more complicated or potentially controversial
change, then the patch submitter will be asked to start a
discussion (if they haven't already) on the mailing list:
http://sourceforge.net/mailarchive/forum.php?forum_name=bitcoin-development

The patch will be accepted if there is broad consensus that it is a
good thing.  Developers should expect to rework and resubmit patches
if they don't match the project's coding conventions (see coding.txt)
or are controversial.

The master branch is regularly built and tested, but is not guaranteed
to be completely stable. Tags are regularly created to indicate new
official, stable release versions of Bitcoin.

Testing
=======

Testing and code review is the bottleneck for development; we get more
pull requests than we can review and test. Please be patient and help
out, and remember this is a security-critical project where any
mistake might cost people lots of money.

Automated Testing
-----------------

Developers are strongly encouraged to write unit tests for new code,
and to submit new unit tests for old code.

Unit tests for the core code are in src/test/
To compile and run them:
  cd src; make -f makefile.linux test

Unit tests for the GUI code are in src/qt/test/
To compile and run them:
  qmake BITCOIN_QT_TEST=1 -o Makefile.test bitcoin-qt.pro
  make -f Makefile.test
  ./Bitcoin-Qt

Every pull request is built for both Windows and
Linux on a dedicated server, and unit and sanity
tests are automatically run. The binaries 
produced may be used for manual QA testing
(a link to them will appear in a comment on the pull request
from 'BitcoinPullTester').
See https://github.com/TheBlueMatt/test-scripts for the
build/test scripts.

Manual Quality Assurance (QA) Testing
-------------------------------------

Large changes should have a test plan, and should be tested
by somebody other than the developer who wrote the code.

See https://github.com/bitcoin/QA/ for how to create a test plan.
