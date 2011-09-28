
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
official, stable release versions of Bitcoin. If you would like to
help test the Bitcoin core, please contact QA@BitcoinTesting.org.

Feature branches are created when there are major new features being
worked on by several people.

From time to time a pull request will become outdated. If this occurs, and
the pull is no longer automatically mergeable; a comment on the pull will
be used to issue a warning of closure. The pull will be closed 15 days
after the warning if action is not taken by the author. Pull requests closed
in this manner will have their corresponding issue labeled 'stagnant'.

Issues with no commits will be given a similar warning, and closed after
15 days from their last activity. Issues closed in this manner will be 
labeled 'stale'. 

Requests to reopen closed pull requests and/or issues can be submitted to 
QA@BitcoinTesting.org. 