#!/bin/sh
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

DIR=$(dirname "$0")
[ "/${DIR#/}" != "$DIR" ] && DIR=$(dirname "$(pwd)/$0")

echo "Using verify-commits data from ${DIR}"

VERIFIED_ROOT=$(cat "${DIR}/trusted-git-root")
VERIFIED_SHA512_ROOT=$(cat "${DIR}/trusted-sha512-root-commit")
REVSIG_ALLOWED=$(cat "${DIR}/allow-revsig-commits")

HAVE_GNU_SHA512=1
[ ! -x "$(which sha512sum)" ] && HAVE_GNU_SHA512=0

if [ x"$1" = "x" ]; then
	CURRENT_COMMIT="HEAD"
else
	CURRENT_COMMIT="$1"
fi

if [ "${CURRENT_COMMIT#* }" != "$CURRENT_COMMIT" ]; then
	echo "Commit must not contain spaces?" > /dev/stderr
	exit 1
fi

VERIFY_TREE=0
if [ x"$2" = "x--tree-checks" ]; then
	VERIFY_TREE=1
fi

NO_SHA1=1
PREV_COMMIT=""
INITIAL_COMMIT="${CURRENT_COMMIT}"

while true; do
	if [ "$CURRENT_COMMIT" = $VERIFIED_ROOT ]; then
		echo "There is a valid path from \"$INITIAL_COMMIT\" to $VERIFIED_ROOT where all commits are signed!"
		exit 0
	fi

	if [ "$CURRENT_COMMIT" = $VERIFIED_SHA512_ROOT ]; then
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

	if [ "${REVSIG_ALLOWED#*$CURRENT_COMMIT}" != "$REVSIG_ALLOWED" ]; then
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=1
	else
		export BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG=0
	fi

	if ! git -c "gpg.program=${DIR}/gpg.sh" verify-commit "$CURRENT_COMMIT" > /dev/null; then
		if [ "$PREV_COMMIT" != "" ]; then
			echo "No parent of $PREV_COMMIT was signed with a trusted key!" > /dev/stderr
			echo "Parents are:" > /dev/stderr
			PARENTS=$(git show -s --format=format:%P $PREV_COMMIT)
			for PARENT in $PARENTS; do
				git show -s $PARENT > /dev/stderr
			done
		else
			echo "$CURRENT_COMMIT was not signed with a trusted key!" > /dev/stderr
		fi
		exit 1
	fi

	# We always verify the top of the tree
	if [ "$VERIFY_TREE" = 1 -o "$PREV_COMMIT" = "" ]; then
		IFS_CACHE="$IFS"
		IFS='
'
		for LINE in $(git ls-tree --full-tree -r "$CURRENT_COMMIT"); do
			case "$LINE" in
				"12"*)
					echo "Repo contains symlinks" > /dev/stderr
					IFS="$IFS_CACHE"
					exit 1
					;;
			esac
		done
		IFS="$IFS_CACHE"

		FILE_HASHES=""
		for FILE in $(git ls-tree --full-tree -r --name-only "$CURRENT_COMMIT" | LC_ALL=C sort); do
			if [ "$HAVE_GNU_SHA512" = 1 ]; then
				HASH=$(git cat-file blob "$CURRENT_COMMIT":"$FILE" | sha512sum | { read FIRST _; echo $FIRST; } )
			else
				HASH=$(git cat-file blob "$CURRENT_COMMIT":"$FILE" | shasum -a 512 | { read FIRST _; echo $FIRST; } )
			fi
			[ "$FILE_HASHES" != "" ] && FILE_HASHES="$FILE_HASHES"'
'
			FILE_HASHES="$FILE_HASHES$HASH  $FILE"
		done

		if [ "$HAVE_GNU_SHA512" = 1 ]; then
			TREE_HASH="$(echo "$FILE_HASHES" | sha512sum)"
		else
			TREE_HASH="$(echo "$FILE_HASHES" | shasum -a 512)"
		fi
		HASH_MATCHES=0
		MSG="$(git show -s --format=format:%B "$CURRENT_COMMIT" | tail -n1)"

		case "$MSG  -" in
			"Tree-SHA512: $TREE_HASH")
				HASH_MATCHES=1;;
		esac

		if [ "$HASH_MATCHES" = "0" ]; then
			echo "Tree-SHA512 did not match for commit $CURRENT_COMMIT" > /dev/stderr
			exit 1
		fi
	fi

	PARENTS=$(git show -s --format=format:%P "$CURRENT_COMMIT")
	for PARENT in $PARENTS; do
		PREV_COMMIT="$CURRENT_COMMIT"
		CURRENT_COMMIT="$PARENT"
		break
	done
done
