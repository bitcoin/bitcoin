#!/usr/bin/env bash
#
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for new lines in diff that introduce trailing whitespace.

# We can't run this check unless we know the commit range for the PR.

export LC_ALL=C
while getopts "?" opt; do
  case $opt in
    ?)
      echo "Usage: $0 [N]"
      echo "       COMMIT_RANGE='<commit range>' $0"
      echo "       $0 -?"
      echo "Checks unstaged changes, the previous N commits, or a commit range."
      echo "COMMIT_RANGE='47ba2c3...ee50c9e' $0"
      exit 0
    ;;
  esac
done

# TRAVIS_BRANCH will be present in a Travis environment. For builds triggered
# by a pull request this is the name of the branch targeted by the pull request.
# https://docs.travis-ci.com/user/environment-variables/
if [ -n "${TRAVIS_BRANCH}" ]; then
  COMMIT_RANGE="$TRAVIS_BRANCH..HEAD"
fi

if [ -z "${COMMIT_RANGE}" ]; then
  if [ -n "$1" ]; then
    COMMIT_RANGE="HEAD~$1...HEAD"
  else
    # This assumes that the target branch of the pull request will be master.
    MERGE_BASE=$(git merge-base HEAD master)
    COMMIT_RANGE="$MERGE_BASE..HEAD"
  fi
fi

showdiff() {
  if ! git diff -U0 "${COMMIT_RANGE}" -- "." ":(exclude)depends/patches/" ":(exclude)src/leveldb/" ":(exclude)src/crc32c/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/" ":(exclude)doc/release-notes/" ":(exclude)src/qt/locale/"; then
    echo "Failed to get a diff"
    exit 1
  fi
}

showcodediff() {
  if ! git diff -U0 "${COMMIT_RANGE}" -- *.cpp *.h *.md *.py *.sh ":(exclude)src/leveldb/" ":(exclude)src/crc32c/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/" ":(exclude)doc/release-notes/" ":(exclude)src/qt/locale/"; then
    echo "Failed to get a diff"
    exit 1
  fi
}

RET=0

# Check if trailing whitespace was found in the diff.
if showdiff | grep -E -q '^\+.*\s+$'; then
  echo "This diff appears to have added new lines with trailing whitespace."
  echo "The following changes were suspected:"
  FILENAME=""
  SEEN=0
  SEENLN=0
  while read -r line; do
    if [[ "$line" =~ ^diff ]]; then
      FILENAME="$line"
      SEEN=0
    elif [[ "$line" =~ ^@@ ]]; then
      LINENUMBER="$line"
      SEENLN=0
    else
      if [ "$SEEN" -eq 0 ]; then
        # The first time a file is seen with trailing whitespace, we print the
        # filename (preceded by a newline).
        echo
        echo "$FILENAME"
        SEEN=1
      fi
      if [ "$SEENLN" -eq 0 ]; then
        echo "$LINENUMBER"
        SEENLN=1
      fi
      echo "$line"
    fi
  done < <(showdiff | grep -E '^(diff --git |@@|\+.*\s+$)')
  RET=1
fi

# Check if tab characters were found in the diff.
if showcodediff | perl -nle '$MATCH++ if m{^\+.*\t}; END{exit 1 unless $MATCH>0}' > /dev/null; then
  echo "This diff appears to have added new lines with tab characters instead of spaces."
  echo "The following changes were suspected:"
  FILENAME=""
  SEEN=0
  SEENLN=0
  while read -r line; do
    if [[ "$line" =~ ^diff ]]; then
      FILENAME="$line"
      SEEN=0
    elif [[ "$line" =~ ^@@ ]]; then
      LINENUMBER="$line"
      SEENLN=0
    else
      if [ "$SEEN" -eq 0 ]; then
        # The first time a file is seen with a tab character, we print the
        # filename (preceded by a newline).
        echo
        echo "$FILENAME"
        SEEN=1
      fi
      if [ "$SEENLN" -eq 0 ]; then
        echo "$LINENUMBER"
        SEENLN=1
      fi
      echo "$line"
    fi
  done < <(showcodediff | perl -nle 'print if m{^(diff --git |@@|\+.*\t)}')
  RET=1
fi

exit $RET
