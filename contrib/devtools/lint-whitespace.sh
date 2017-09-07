#!/bin/bash
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for new lines in diff that introduce trailing whitespace.

# We can't run this check unless we know the commit range for the PR.
if [ -z "${TRAVIS_COMMIT_RANGE}" ]; then
  exit 0
fi

showdiff() {
  if ! git diff -U0 "${TRAVIS_COMMIT_RANGE}" --; then
    echo "Failed to get a diff"
    exit 1
  fi
}

# Do a first pass, and if no trailing whitespace was found then exit early.
if ! showdiff | grep -E -q '^\+.*\s+$'; then
  exit
fi

echo "This diff appears to have added new lines with trailing whitespace."
echo "The following changes were suspected:"

FILENAME=""
SEEN=0

while read -r line; do
  if [[ "$line" =~ ^diff ]]; then
    FILENAME="$line"
    SEEN=0
  else
    if [ "$SEEN" -eq 0 ]; then
      # The first time a file is seen with trailing whitespace, we print the
      # filename (preceded by a newline).
      echo
      echo "$FILENAME"
      SEEN=1
    fi
    echo "$line"
  fi
done < <(showdiff | grep -E '^(diff --git |\+.*\s+$)')
exit 1
