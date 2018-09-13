#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Warn in case of spelling errors.
# Note: Will exit successfully regardless of spelling errors.

export LC_ALL=C

EXIT_CODE=0
IGNORE_WORDS_FILE=test/lint/lint-spelling.ignore-words.txt
if ! codespell --check-filenames --disable-colors --quiet-level=7 --ignore-words=${IGNORE_WORDS_FILE} $(git ls-files -- ":(exclude)build-aux/m4/" ":(exclude)contrib/seeds/*.txt" ":(exclude)depends/" ":(exclude)doc/release-notes/" ":(exclude)src/leveldb/" ":(exclude)src/qt/locale/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/"); then
    echo "^ Warning: codespell identified likely spelling errors. Any false positives? Add them to the list of ignored words in ${IGNORE_WORDS_FILE}"
    EXIT_CODE=1
fi
exit ${EXIT_CODE}
