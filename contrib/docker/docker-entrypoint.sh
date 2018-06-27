#!/bin/bash
set -e

if [[ "$1" = "bitcoind" ]]; then
    exec gosu bitcoin bitcoind ${*:2}
elif [[ "$1" == "bitcoin-cli" ]]; then
    exec gosu bitcoin bitcoin-cli ${*:2}
elif [[ "$1" == "bitcoin-tx" ]]; then
    exec gosu bitcoin bitcoin-tx ${*:2}
else
    exec "$@"
fi
