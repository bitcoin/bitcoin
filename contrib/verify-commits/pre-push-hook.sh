#!/bin/bash
if ! [[ "$2" =~ ^(git@)?(www.)?github.com(:|/)bitcoin/bitcoin(.git)?$ ]]; then
    exit 0
fi

while read LINE; do
    set -- A $LINE
    if [ "$4" != "refs/heads/master" ]; then
        continue
    fi
    if ! ./contrib/verify-commits/verify-commits.sh $3 > /dev/null 2>&1; then
        echo "ERROR: A commit is not signed, can't push"
        ./contrib/verify-commits/verify-commits.sh
        exit 1
    fi
done < /dev/stdin
