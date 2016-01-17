Contributing to Bitcoin Core
============================

The Bitcoin Core project operates an open contributor model where anyone is welcome to contribute towards development in the form of peer review, testing and patches. This document explains the practical process and guidelines for contributing.

Firstly in terms of structure, there is no particular concept of “Core developers” in the sense of privileged people. Open source often naturally revolves around meritocracy where longer term contributors gain more trust from the developer community. However, some hierarchy is necessary for practical purposes. As such there are repository “maintainers” who are responsible for merging pull requests as well as a “lead maintainer” who is responsible for the release cycle, overall merging, moderation and appointment of maintainers.


Contributor Workflow
--------------------

The codebase is maintained using the “contributor workflow” where everyone without exception contributes patch proposals using “pull requests”. This facilitates social contribution, easy testing and peer review.

To contribute a patch, the workflow is as follows:

  - Fork repository
  - Create topic branch
  - Commit patches

The project coding conventions in [doc/developer-notes.md](doc/developer-notes.md) must be adhered to.

In general [commits should be atomic](https://en.wikipedia.org/wiki/Atomic_commit#Atomic_commit_convention) and diffs should be easy to read. For this reason do not mix any formatting fixes or code moves with actual code changes.

Commit messages should be verbose by default consisting of a short subject line (50 chars max), a blank line and detailed explanatory text as separate paragraph(s); unless the title alone is self-explanatory (like "Corrected typo in main.cpp") then a single title line is sufficient. Commit messages should be helpful to people reading your code in the future, so explain the reasoning for your decisions. Further explanation [here](http://chris.beams.io/posts/git-commit/).

If a particular commit references another issue, please add the reference, for example "refs #1234", or "fixes #4321". Using "fixes or closes" keywords will cause the corresponding issue to be closed when the pull request is merged.

Please refer to the [Git manual](https://git-scm.com/doc) for more information about Git.

  - Push changes to your fork
  - Create pull request

The title of the pull request should be prefixed by the component or area that the pull request affects. Examples:

    Consensus: Add new opcode for BIP-XXXX OP_CHECKAWESOMESIG
    Net: Automatically create hidden service, listen on Tor
    Qt: Add feed bump button
    Trivial: fix typo

If a pull request is specifically not to be considered for merging (yet) please prefix the title with [WIP] or use [Tasks Lists](https://github.com/blog/1375-task-lists-in-gfm-issues-pulls-comments) in the body of the pull request to indicate tasks are pending.

The body of the pull request should contain enough description about what the patch does together with any justification/reasoning. You should include references to any discussions (for example other tickets or mailing list discussions).

At this stage one should expect comments and review from other contributors. You can add more commits to your pull request by committing them locally and pushing to your fork until you have satisfied all feedback. If your pull request is accepted for merging, you may be asked by a maintainer to squash and or rebase your commits before it will be merged. The length of time required for peer review is unpredictable and will vary from patch to patch.


Pull Request Philosophy
-----------------------

Patchsets should always be focused. For example, a pull request could add a feature, fix a bug, or refactor code; but not a mixture. Please also avoid super pull requests which attempt to do too much, are overly large, or overly complex as this makes review difficult.


###Features

When adding a new feature, thought must be given to the long term technical debt and maintenance that feature may require after inclusion. Before proposing a new feature that will require maintenance, please consider if you are willing to maintain it (including bug fixing). If features get orphaned with no maintainer in the future, they may be removed by the Repository Maintainer.


###Refactoring

Refactoring is a necessary part of any software project's evolution. The following guidelines cover refactoring pull requests for the project.

There are three categories of refactoring, code only moves, code style fixes, code refactoring. In general refactoring pull requests should not mix these three kinds of activity in order to make refactoring pull requests easy to review and uncontroversial. In all cases, refactoring PRs must not change the behaviour of code within the pull request (bugs must be preserved as is).

Project maintainers aim for a quick turnaround on refactoring pull requests, so where possible keep them short, uncomplex and easy to verify. 


"Decision Making" Process
-------------------------

TBD.

Relevant links:

- https://github.com/bitcoinclassic/bitcoinclassic/issues/13
- https://bitcoinclassic.consider.it/how-to-change-the-code?results=true

Release Policy
--------------

The project leader is the release manager for each Bitcoin Core release.
