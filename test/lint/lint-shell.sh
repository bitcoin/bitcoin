#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for shellcheck warnings in shell scripts.

export LC_ALL=C

# Disabled warnings:
disabled=(
    SC2046 # Quote this to prevent word splitting.
    SC2086 # Double quote to prevent globbing and word splitting.
    SC2162 # read without -r will mangle backslashes.
)

EXIT_CODE=0

if ! command -v shellcheck > /dev/null; then
    echo "Skipping shell linting since shellcheck is not installed."
    exit $EXIT_CODE
fi

SHELLCHECK_CMD=(shellcheck --external-sources --check-sourced)
EXCLUDE="--exclude=$(IFS=','; echo "${disabled[*]}")"
SOURCED_FILES=$(git ls-files | xargs gawk '/^# shellcheck shell=/ {print FILENAME} {nextfile}')  # Check shellcheck directive used for sourced files
if ! "${SHELLCHECK_CMD[@]}" "$EXCLUDE" $SOURCED_FILES $(git ls-files -- '*.sh' | grep -vE 'src/(leveldb|secp256k1|univalue)/'); then
    EXIT_CODE=1
fi

exit $EXIT_CODE
