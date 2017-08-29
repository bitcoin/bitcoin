Contents
========
This directory contains tools for developers working on this repository.

clang-format.py
===============

A script to format cpp source code according to [.clang-format](../../src/.clang-format). This should only be applied to new files or files which are currently not actively developed on. Also, git subtrees are not subject to formatting.

fix-copyright-headers.py
========================

Every year newly updated files need to have its copyright headers updated to reflect the current year.
If you run this script from the root folder it will automatically update the year on the copyright header for all
source files if these have a git commit from the current year.

For example a file changed in 2015 (with 2015 being the current year):

```// Copyright (c) 2009-2013 The Syscoin Core developers```

would be changed to:

```// Copyright (c) 2009-2015 The Syscoin Core developers```

git-subtree-check.sh
====================

Run this script from the root of the repository to verify that a subtree matches the contents of
the commit it claims to have been updated to.

To use, make sure that you have fetched the upstream repository branch in which the subtree is
maintained:
* for `src/secp256k1`: https://github.com/syscoin/secp256k1.git (branch master)
* for `src/leveldb`: https://github.com/syscoin/leveldb.git (branch syscoin-fork)
* for `src/univalue`: https://github.com/syscoin/univalue.git (branch master)

Usage: `git-subtree-check.sh DIR COMMIT`

`COMMIT` may be omitted, in which case `HEAD` is used.

github-merge.sh
===============

A small script to automate merging pull-requests securely and sign them with GPG.

For example:

  ./github-merge.sh syscoin/syscoin 3077

(in any git repository) will help you merge pull request #3077 for the
syscoin/syscoin repository.

What it does:
* Fetch master and the pull request.
* Locally construct a merge commit.
* Show the diff that merge results in.
* Ask you to verify the resulting source tree (so you can do a make
check or whatever).
* Ask you whether to GPG sign the merge commit.
* Ask you whether to push the result upstream.

This means that there are no potential race conditions (where a
pullreq gets updated while you're reviewing it, but before you click
merge), and when using GPG signatures, that even a compromised github
couldn't mess with the sources.

Setup
---------
Configuring the github-merge tool for the syscoin repository is done in the following way:

    git config githubmerge.repository syscoin/syscoin
    git config githubmerge.testcmd "make -j4 check" (adapt to whatever you want to use for testing)
    git config --global user.signingkey mykeyid (if you want to GPG sign)

optimize-pngs.py
================

A script to optimize png files in the syscoin
repository (requires pngcrush).

security-check.py and test-security-check.py
============================================

Perform basic ELF security checks on a series of executables.

symbol-check.py
===============

A script to check that the (Linux) executables produced by gitian only contain
allowed gcc, glibc and libstdc++ version symbols. This makes sure they are
still compatible with the minimum supported Linux distribution versions.

Example usage after a gitian build:

    find ../gitian-builder/build -type f -executable | xargs python contrib/devtools/symbol-check.py 

If only supported symbols are used the return value will be 0 and the output will be empty.

If there are 'unsupported' symbols, the return value will be 1 a list like this will be printed:

    .../64/test_syscoin: symbol memcpy from unsupported version GLIBC_2.14
    .../64/test_syscoin: symbol __fdelt_chk from unsupported version GLIBC_2.15
    .../64/test_syscoin: symbol std::out_of_range::~out_of_range() from unsupported version GLIBCXX_3.4.15
    .../64/test_syscoin: symbol _ZNSt8__detail15_List_nod from unsupported version GLIBCXX_3.4.15

update-translations.py
======================

Run this script from the root of the repository to update all translations from transifex.
It will do the following automatically:

- fetch all translations
- post-process them into valid and committable format
- add missing translations to the build system (TODO)

See doc/translation-process.md for more information.
