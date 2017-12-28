#!/bin/sh
INPUT=$(cat /dev/stdin)
VALID=false
REVSIG=false
IFS='
'
for LINE in $(echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null); do
	case "$LINE" in
	"[GNUPG:] VALIDSIG "*)
		while read KEY; do
			case "$LINE" in "[GNUPG:] VALIDSIG $KEY "*) VALID=true;; esac
		done < ./contrib/verify-commits/trusted-keys
		;;
	"[GNUPG:] REVKEYSIG "*)
		[ "$BITCOIN_VERIFY_COMMITS_ALLOW_REVSIG" != 1 ] && exit 1
		while read KEY; do
			case "$LINE" in "[GNUPG:] REVKEYSIG ${KEY#????????????????????????} "*)
				REVSIG=true
				GOODREVSIG="[GNUPG:] GOODSIG ${KEY#????????????????????????} "
			esac
		done < ./contrib/verify-commits/trusted-keys
		;;
	esac
done
if ! $VALID; then
	exit 1
fi
if $VALID && $REVSIG; then
	echo "$INPUT" | gpg --trust-model always "$@" | grep "\[GNUPG:\] \(NEWSIG\|SIG_ID\|VALIDSIG\)" 2>/dev/null
	echo "$GOODREVSIG"
else
	echo "$INPUT" | gpg --trust-model always "$@" 2>/dev/null
fi
