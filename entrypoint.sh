#!/bin/bash

set -e

case "$PROTOCOL" in
    IPV6)
        RPC_BIND="-rpcbind=[::]:8332"
        RPC_ALLOW="-rpcallowip=::/0"
        ;;
    IPV4|"")
        RPC_BIND="-rpcbind=0.0.0.0:8332"
        RPC_ALLOW="-rpcallowip=0.0.0.0/0"
        ;;
    *)
        echo "Protocolo inválido: $PROTOCOL (use IPV4 ou IPV6)"
        exit 1
        ;;
esac

CMD_ARGS="-server=1 -printtoconsole -datadir=$HOME/.bitcoin -disablewallet $RPC_BIND $RPC_ALLOW"

if [ "$(id -u)" = '0' ]; then
    chown -R 1000:1000 $HOME/.bitcoin
    exec gosu bitcoin bitcoind $CMD_ARGS "$@"
else
    exec bitcoind $CMD_ARGS "$@"
fi
