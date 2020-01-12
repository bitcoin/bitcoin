#!/bin/bash
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for new lines in diff that introduce trailing whitespace.

# We can't run this check unless we know the commit range for the PR.
if [ -z "${COMMIT_RANGE}" ]; then
  echo "Cannot run lint-whitespace.sh without commit range. To run locally, use:"
  echo "COMMIT_RANGE='<commit range>' .lint-whitespace.sh"
  echo "For example:"
  echo "COMMIT_RANGE='47ba2c3...ee50c9e' .lint-whitespace.sh"
  exit 1
fi

showdiff() {
  if ! git diff -U0 "${COMMIT_RANGE}" -- "." ":(exclude)src/leveldb/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/" ":(exclude)doc/release-notes/"; then
    echo "Failed to get a diff"
    exit 1
  fi
}

showcodediff() {
  if ! git diff -U0 "${COMMIT_RANGE}" -- *.cpp *.h *.md *.py *.sh ":(exclude)src/leveldb/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/" ":(exclude)doc/release-notes/"; then
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
  while read -r line; do
    if [[ "$line" =~ ^diff ]]; then
      FILENAME="$line"
      SEEN=0
    elif [[ "$line" =~ ^@@ ]]; then
      LINENUMBER="$line"
    else
      if [ "$SEEN" -eq 0 ]; then
        # The first time a file is seen with trailing whitespace, we print the
        # filename (preceded by a newline).
        echo
        echo "$FILENAME"
        echo "$LINENUMBER"
        SEEN=1
      fi
      echo "$line"
    fi
  done < <(showdiff | grep -E '^(diff --git |@@|\+.*\s+$)')
  RET=1
fi

# Check if tab characters were found in the diff.
if showcodediff | grep -P -q '^\+.*\t'; then
  echo "This diff appears to have added new lines with tab characters instead of spaces."
  echo "The following changes were suspected:"
  FILENAME=""
  SEEN=0
  while read -r line; do
    if [[ "$line" =~ ^diff ]]; then
      FILENAME="$line"
      SEEN=0
    elif [[ "$line" =~ ^@@ ]]; then
      LINENUMBER="$line"
    else
      if [ "$SEEN" -eq 0 ]; then
        # The first time a file is seen with a tab character, we print the
        # filename (preceded by a newline).
        echo
        echo "$FILENAME"
        echo "$LINENUMBER"
        SEEN=1
      fi
      echo "$line"
    fi
  done < <(showcodediff | grep -P '^(diff --git |@@|\+.*\t)')
  RET=1
fi

exit $RET
