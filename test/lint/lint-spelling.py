#!/usr/bin/env python3
#
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Warn in case of spelling errors.
Note: Will exit successfully regardless of spelling errors.
"""

from subprocess import check_output, STDOUT, CalledProcessError

from lint_ignore_dirs import SHARED_EXCLUDED_SUBTREES

IGNORE_WORDS_FILE = 'test/lint/spelling.ignore-words.txt'
FILES_ARGS = ['git', 'ls-files', '--', ":(exclude)build-aux/m4/", ":(exclude)contrib/seeds/*.txt", ":(exclude)depends/", ":(exclude)doc/release-notes/", ":(exclude)src/qt/locale/", ":(exclude)src/qt/*.qrc", ":(exclude)contrib/guix/patches"]
FILES_ARGS += [f":(exclude){dir}" for dir in SHARED_EXCLUDED_SUBTREES]


def check_codespell_install():
    try:
        check_output(["codespell", "--version"])
    except FileNotFoundError:
        print("Skipping spell check linting since codespell is not installed.")
        exit(0)


def main():
    check_codespell_install()

    files = check_output(FILES_ARGS).decode("utf-8").splitlines()
    codespell_args = ['codespell', '--check-filenames', '--disable-colors', '--quiet-level=7', '--ignore-words={}'.format(IGNORE_WORDS_FILE)] + files

    try:
        check_output(codespell_args, stderr=STDOUT)
    except CalledProcessError as e:
        print(e.output.decode("utf-8"), end="")
        print('^ Warning: codespell identified likely spelling errors. Any false positives? Add them to the list of ignored words in {}'.format(IGNORE_WORDS_FILE))


if __name__ == "__main__":
    main()
