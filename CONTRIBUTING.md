# Contributing to Bitcoin Classic

The Bitcoin project operates an open contributor model where anyone is welcome
to contribute towards development in the form of peer review, testing and
changes. This document explains the practical process and guidelines for
contributing.


# Workflow

In the Classic project all contributions are valuable and everyone can
contribute.  Contributions are valued based on how well they fit in the project
goals and spirit.  It helps to socialize with the Classic people if you are
unsure what those are.

To contribute to the codebase you can make a github "pull request". This
process is described in githubs documentation.

In classic we follow the well known git branching model where we have a stable
and a development branch, called master.

## 1.2 (stable branch)

Any merge request to this branch is meant to go into the next release. Any
commits made here will be merged into the master branch by a volunteer.

This is the stable branch for a reason, we essentially should be able to
release from this branch fairly quickly at any time. So no unfinished code
reaches this branch.

## Master branch

The master branch is where your merge requests go for finished stuff,
that you want the world to see and that you want others to work on. But may
take a bit more time to stabilize.

The basic idea is that we accept pull requests rather optimistically. The aim is
to merge quickly often and follow up on changes should they have issues,
don't be afraid to fix other peoples code and don't take it personally if
someone else makes a change to your feature.

The Stable branch is where bugfixes go, after plenty of review. And in Git
terminology that branch is free to merge into yours.


# To be aware of;

The project coding conventions in [doc/developer-notes.md](doc/developer-notes.md)
must be adhered to.

In general [commits should be atomic](https://en.wikipedia.org/wiki/Atomic_commit#Atomic_commit_convention)
and diffs should be easy to read. For this reason do not mix any formatting fixes
or code moves with actual code changes.

Commit messages follow the git standard.  
This means a single line, short comment. A blank line and then a detailed
description below, if needed.

Please refer to the [Git manual](https://git-scm.com/doc) for more information about Git.


Pull Request Philosophy
-----------------------

Patchsets should always be focused. For example, a pull request could add a
feature, fix a bug, or refactor code; but not a mixture. Please also avoid super
pull requests which attempt to do too much, are overly large, or overly complex
as this makes review difficult.
