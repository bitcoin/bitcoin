#!/bin/sh
INPUT=$(</dev/stdin)
VALID=false
IFS=$'\n'
for LINE in $(echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null); do
	case "$LINE" in "[GNUPG:] VALIDSIG"*)
		while read KEY; do
			case "$LINE" in "[GNUPG:] VALIDSIG $KEY "*) VALID=true;; esac
		done < ./contrib/verify-commits/trusted-keys
	esac
done
if ! $VALID; then
	exit 1
fi
echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null
