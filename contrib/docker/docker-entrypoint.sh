#!/usr/bin/env bash
export LC_ALL=C
set -e

chown -R coinuser "${COIN_ROOT_DIR}/"

if [ "$(echo "$1" | cut -c1)" = "-" ]; then
  set -- bitcoind "$@"
fi

if [ "$1" = "bitcoind" ] || [ "$1" = "bitcoin-cli" ]; then
  # Set config file
  set -- "$1" -conf="$COIN_CONF_FILE" "${@:2}"
fi

if [ "$1" = "bitcoind" ]; then
  # Set config file
  set -- "$@" -printtoconsole
fi

echo "$@"
if [ "$1" = "bitcoind" ] || [ "$1" = "bitcoin-cli" ] || [ "$1" = "bitcoin-tx" ]; then
  exec gosu coinuser "$@"
else
  exec "$@"
fi
