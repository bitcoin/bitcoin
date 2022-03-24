#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Warn in case of spelling errors.
# Note: Will exit successfully regardless of spelling errors.

export LC_ALL=C

IGNORED_SUBTREES_REGEXP=$(sed -E 's/^/src\//g' test/lint/lint-ignored-subtrees.txt | \
    sed -E 's/$/\//g' | \
    tr '\n' '|')

if ! command -v codespell > /dev/null; then
    echo "Skipping spell check linting since codespell is not installed."
    exit 0
fi

IGNORE_WORDS_FILE=test/lint/lint-spelling.ignore-words.txt
mapfile -t FILES < <(git ls-files -- | grep -Ev "build-aux/m4/|contrib/seeds/.*\.txt|depends/|doc/release-notes|src/qt/locale/|src/qt/.*\.qrc|contrib/builder-keys/keys\.txt|contrib/guix/patches|${IGNORED_SUBTREES_REGEXP}")
if ! codespell --check-filenames --disable-colors --quiet-level=7 --ignore-words=${IGNORE_WORDS_FILE} "${FILES[@]}"; then
    echo "^ Warning: codespell identified likely spelling errors. Any false positives? Add them to the list of ignored words in ${IGNORE_WORDS_FILE}"
fi
