#!/bin/sh
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Not technically POSIX-compliant due to use of "local", but almost every
# shell anyone uses today supports it, so its probably fine

DIR=$(dirname "$0")
[ "/${DIR#/}" != "$DIR" ] && DIR=$(dirname "$(pwd)/$0")

VERIFIED_ROOT=$(cat "${DIR}/trusted-git-root")
REVSIG_ALLOWED=$(cat "${DIR}/allow-revsig-commits")

HAVE_FAILED=false
IS_SIGNED () {
	if [ $1 = $VERIFIED_ROOT ]; then
		return 0;
	fi
	if [ "${REVSIG_ALLOWED#*$1}" != "$REVSIG_ALLOWED" ]; then
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=1
	else
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=0
	fi
	if ! git -c "gpg.program=${DIR}/gpg.sh" verify-commit $1 > /dev/null 2>&1; then
		return 1;
	fi
	local PARENTS
	PARENTS=$(git show -s --format=format:%P $1)
	for PARENT in $PARENTS; do
		if IS_SIGNED $PARENT; then
			return 0;
		fi
		break
	done
	if ! "$HAVE_FAILED"; then
		echo "No parent of $1 was signed with a trusted key!" > /dev/stderr
		echo "Parents are:" > /dev/stderr
		for PARENT in $PARENTS; do
			git show -s $PARENT > /dev/stderr
		done
		HAVE_FAILED=true
	fi
	return 1;
}

if [ x"$1" = "x" ]; then
	TEST_COMMIT="HEAD"
else
	TEST_COMMIT="$1"
fi

IS_SIGNED "$TEST_COMMIT"
RES=$?
if [ "$RES" = 1 ]; then
	if ! "$HAVE_FAILED"; then
		echo "$TEST_COMMIT was not signed with a trusted key!"
	fi
else
	echo "There is a valid path from $TEST_COMMIT to $VERIFIED_ROOT where all commits are signed!"
fi

exit $RES
