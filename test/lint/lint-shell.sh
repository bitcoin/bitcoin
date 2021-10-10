#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for shellcheck warnings in shell scripts.

export LC_ALL=C

# The shellcheck binary segfault/coredumps in Travis with LC_ALL=C
# It does not do so in Ubuntu 14.04, 16.04, 18.04 in versions 0.3.3, 0.3.7, 0.4.6
# respectively. So export LC_ALL=C is set as required by lint-shell-locale.sh
# but unset here in case of running in Travis.
if [ "$TRAVIS" = "true" ]; then
    unset LC_ALL
fi

# Disabled warnings:
disabled=(
    SC1087 # Use braces when expanding arrays, e.g. ${array[idx]} (or ${var}[.. to quiet).
    SC2001 # See if you can use ${variable//search/replace} instead.
    SC2004 # $/${} is unnecessary on arithmetic variables.
    SC2005 # Useless echo? Instead of 'echo $(cmd)', just use 'cmd'.
    SC2006 # Use $(..) instead of legacy `..`.
    SC2016 # Expressions don't expand in single quotes, use double quotes for that.
    SC2028 # echo won't expand escape sequences. Consider printf.
    SC2046 # Quote this to prevent word splitting.
    SC2048 # Use "$@" (with quotes) to prevent whitespace problems.
    SC2066 # Since you double quoted this, it will not word split, and the loop will only run once.
    SC2086 # Double quote to prevent globbing and word splitting.
    SC2116 # Useless echo? Instead of 'cmd $(echo foo)', just use 'cmd foo'.
    SC2162 # read without -r will mangle backslashes.
    SC2166 # Prefer [ p ] {&&,||} [ q ] as [ p -{a,o} q ] is not well defined.
    SC2181 # Check exit code directly with e.g. 'if mycmd;', not indirectly with $?.
    SC2206 # Quote to prevent word splitting, or split robustly with mapfile or read -a.
    SC2207 # Prefer mapfile or read -a to split command output (or quote to avoid splitting).
    SC2230 # which is non-standard. Use builtin 'command -v' instead.
    SC2236 # Don't force -n instead of ! -z.
)
disabled_gitian=(
    SC2094 # Make sure not to read and write the same file in the same pipeline.
    SC2129 # Consider using { cmd1; cmd2; } >> file instead of individual redirects.
)

EXIT_CODE=0

if ! command -v shellcheck > /dev/null; then
    echo "Skipping shell linting since shellcheck is not installed."
    exit $EXIT_CODE
fi

if ! command -v gawk > /dev/null; then
    echo "Skipping shell linting since gawk is not installed."
    exit $EXIT_CODE
fi

EXCLUDE="--exclude=$(IFS=','; echo "${disabled[*]}")"
SOURCED_FILES=$(git ls-files | xargs gawk '/^# shellcheck shell=/ {print FILENAME} {nextfile}')  # Check shellcheck directive used for sourced files
if ! shellcheck "$EXCLUDE" $SOURCED_FILES $(git ls-files -- '*.sh' | grep -vE 'src/(leveldb|secp256k1|univalue)/'); then
    EXIT_CODE=1
fi

if ! command -v yq > /dev/null; then
    echo "Skipping Gitian desriptor scripts checking since yq is not installed."
    exit $EXIT_CODE
fi

if ! command -v jq > /dev/null; then
    echo "Skipping Gitian descriptor scripts checking since jq is not installed."
    exit $EXIT_CODE
fi

EXCLUDE_GITIAN=${EXCLUDE}",$(IFS=','; echo "${disabled_gitian[*]}")"
for descriptor in $(git ls-files -- 'contrib/gitian-descriptors/*.yml')
do
    echo
    echo "$descriptor"
    # Use #!/bin/bash as gitian-builder/bin/gbuild does to complete a script.
    SCRIPT=$'#!/bin/bash\n'$(yq -r .script "$descriptor")
    if ! echo "$SCRIPT" | shellcheck "$EXCLUDE_GITIAN" -; then
        EXIT_CODE=1
    fi
done

exit $EXIT_CODE
