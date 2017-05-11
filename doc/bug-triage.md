Triage of bugs
====================================

Occasionally, you may encounter a failing test, or a user reporting a
bug.

If there is no test, a good first step is to get a good reproduction of
the issue, and then write a test for it which fails only if the problem
is present.  Sometimes this is difficult in itself, but try.

If you are lucky, and existing regression test might already flag the
issue (this is rare).

Once you are able to reproduce the problem, the next step is to figure
out where it crept into the software.

Having a script which returns 0 if the problem is not present and
1 if it is present (after some test steps) allows us to automatically
find the Git commit which first introduced the problem (well, this works
*most* of the time - there are difficult edge cases but we will not worry
about those now).


Running a git bisection
------------------------------------

The `git bisect` command can run an automatic bisection if we provide it
with a command or script that can assess whether a revision is defective.

There is a helper script in contrib/testtools/bisect.sh which can act
as a wrapper to find even the otherwise difficult to debug failures, e.g.
tests which fail only randomly, or which enter a never-ending loop.
You need to copy this script to somewhere outside your working area,
so it does not get clobbered when git checks out a different revision.

The usage of calling bisect.sh is described in its file header.
You should read that to understand which parameters you may want to adjust
in your copy, e.g. the number of cores, and whether to reconfigure between
revisions. The latter is important if the build system can change
dramatically during the bisection. In that case the bisect.sh needs to run
a full autogen/configure before building each revision, otherwise a simple
'make' will fail. If in doubt, set the reconfiguration variable to 1.

To start a bisection, check out the revision which is known to be bad.
If you need to, re-run the test manually to convince yourself, using
the bisect.sh wrapper.

Next, find a revision where the test still passes (i.e. a known good
revision). It is often simplest to track back to a previous release
version and test that to ensure that it was still good. Go back as
far as you need.

If the problem is that a test fails randomly and/or loops, you will need to
determine two parameters to the bisection:

1. iterations: after how many successful iterations will you accept that a
revision is good

2. timeout: after how much time will you time out a test iteration and
declare a revision bad

Once you find a good commit, start the bisection:

    $ git bisect start <bad> <good>

where the <bad> and <good> are the commit hashes of the bad and good commits.

Then kick off the automatic bisection:

    $ git bisect run /path/to/bisect.sh <iterations> <timeout> <command>

If you have configured the iterations, timeout and command in your copy
of bisect.sh, the parameters you specify above will override those
settings.

Once git is done bisecting, it will output a message showing the first
commit that is bad.
You need to actually check the bisection output to ensure that the
tests were executed in an orderly manner, otherwise this "bad commit"
is not necessarily the first bad one.

Sometimes git may declare a revision bad because the wrapper script failed
because the revision did not build.

If the revision cannot be assessed (because broken build state) then you
can use 'git bisect skip' to skip such revisions - but read up on the
syntax with 'git help bisect' first.

Introducing skipped revisions can lead git to not be able to tell which
revision is the first bad one. Sometimes it may output multiple
candidates.

You can do a 'git bisect reset' and start over if your bisection script
did not perform adequately (e.g. if the timeout is too short and you
suspect that iterations which would have passed were shot down due to
the timeout.

