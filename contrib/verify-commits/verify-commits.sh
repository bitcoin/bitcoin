#!/bin/sh
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Not technically POSIX-compliant due to use of "local", but almost every
# shell anyone uses today supports it, so its probably fine

DIR=$(dirname "$0")
[ "/${DIR#/}" != "$DIR" ] && DIR=$(dirname "$(pwd)/$0")

echo "Using verify-commits data from ${DIR}"

VERIFIED_ROOT=$(cat "${DIR}/trusted-git-root")
VERIFIED_SHA512_ROOT=$(cat "${DIR}/trusted-sha512-root-commit")
REVSIG_ALLOWED=$(cat "${DIR}/allow-revsig-commits")

HAVE_FAILED=false
IS_SIGNED () {
	if [ $1 = $VERIFIED_ROOT ]; then
		return 0;
	fi

	VERIFY_TREE=$2
	NO_SHA1=$3
	if [ $1 = $VERIFIED_SHA512_ROOT ]; then
		if [ "$VERIFY_TREE" = "1" ]; then
			echo "All Tree-SHA512s matched up to $VERIFIED_SHA512_ROOT" > /dev/stderr
		fi
		VERIFY_TREE=0
		NO_SHA1=0
	fi

	if [ "$NO_SHA1" = "1" ]; then
		export BITCOIN_VERIFY_COMMITS_ALLOW_SHA1=0
	else
		export BITCOIN_VERIFY_COMMITS_ALLOW_SHA1=1
	fi

	if [ "${REVSIG_ALLOWED#*$1}" != "$REVSIG_ALLOWED" ]; then
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=1
	else
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=0
	fi

	if ! git -c "gpg.program=${DIR}/gpg.sh" verify-commit $1 > /dev/null; then
		return 1;
	fi

	# We set $4 to 1 on the first call, always verifying the top of the tree
	if [ "$VERIFY_TREE" = 1 -o "$4" = "1" ]; then
		IFS_CACHE="$IFS"
		IFS='
'
		for LINE in $(git ls-tree --full-tree -r $1); do
			case "$LINE" in
				"12"*)
					echo "Repo contains symlinks" > /dev/stderr
					IFS="$IFS_CACHE"
					return 1
					;;
			esac
		done
		IFS="$IFS_CACHE"

		FILE_HASHES=""
		for FILE in $(git ls-tree --full-tree -r --name-only $1 | LC_ALL=C sort); do
			HASH=$(git cat-file blob $1:"$FILE" | sha512sum | { read FIRST OTHER; echo $FIRST; } )
			[ "$FILE_HASHES" != "" ] && FILE_HASHES="$FILE_HASHES"'
'
			FILE_HASHES="$FILE_HASHES$HASH  $FILE"
		done
		HASH_MATCHES=0
		MSG="$(git show -s --format=format:%B $1 | tail -n1)"

		case "$MSG  -" in
			"Tree-SHA512: $(echo "$FILE_HASHES" | sha512sum)")
				HASH_MATCHES=1;;
		esac

		if [ "$HASH_MATCHES" = "0" ]; then
			echo "Tree-SHA512 did not match for commit $1" > /dev/stderr
			HAVE_FAILED=true
			return 1
		fi
	fi

	local PARENTS
	PARENTS=$(git show -s --format=format:%P $1)
	for PARENT in $PARENTS; do
		if IS_SIGNED $PARENT $VERIFY_TREE $NO_SHA1 0; then
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

DO_CHECKOUT_TEST=0
if [ x"$2" = "x--tree-checks" ]; then
	DO_CHECKOUT_TEST=1
fi

IS_SIGNED "$TEST_COMMIT" "$DO_CHECKOUT_TEST" 1 1
RES=$?
if [ "$RES" = 1 ]; then
	if ! "$HAVE_FAILED"; then
		echo "$TEST_COMMIT was not signed with a trusted key!"
	fi
else
	echo "There is a valid path from $TEST_COMMIT to $VERIFIED_ROOT where all commits are signed!"
fi

exit $RES
