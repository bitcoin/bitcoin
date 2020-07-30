#!/usr/bin/env bash
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Linter to check that commit messages have a new line before the body
# or no body at all

export LC_ALL=C

EXIT_CODE=0

while getopts "?" opt; do
  case $opt in
    ?)
      echo "Usage: $0 [N]"
      echo "       TRAVIS_COMMIT_RANGE='<commit range>' $0"
      echo "       $0 -?"
      echo "Checks unmerged commits, the previous N commits, or a commit range."
      echo "TRAVIS_COMMIT_RANGE='47ba2c3...ee50c9e' $0"
      exit ${EXIT_CODE}
    ;;
  esac
done

if [ -z "${TRAVIS_COMMIT_RANGE}" ]; then
  if [ -n "$1" ]; then
    TRAVIS_COMMIT_RANGE="HEAD~$1...HEAD"
  else
    TRAVIS_COMMIT_RANGE="origin/master..HEAD"
  fi
fi

while IFS= read -r commit_hash  || [[ -n "$commit_hash" ]]; do
    n_line=0
    while IFS= read -r line || [[ -n "$line" ]]; do
        n_line=$((n_line+1))
        length=${#line}
        if [ $n_line -eq 2 ] && [ $length -ne 0 ]; then
            echo "The subject line of commit hash ${commit_hash} is followed by a non-empty line. Subject lines should always be followed by a blank line."
            EXIT_CODE=1
        fi
    done < <(git log --format=%B -n 1 "$commit_hash")
done < <(git log "${TRAVIS_COMMIT_RANGE}" --format=%H)

exit ${EXIT_CODE}
