Docker setup
-------------------

## Requirements

Docker engine release: 18.02.0+
docker-compose: 1.20.0+

## Build and start

`docker-compose -p bitcoin up -d`

## Check status

`docker-compose -p bitcoin ps`

### Output
```
     Name                   Command               State                                                  Ports                                                
--------------------------------------------------------------------------------------------------------------------------------------------------------------
bitcoin_node_1   /docker-entrypoint.sh bitc ...   Up      0.0.0.0:32833->18332/tcp, 0.0.0.0:32832->18333/tcp, 0.0.0.0:32835->8332/tcp, 0.0.0.0:32834->8333/tcp
```

## Jump into container

As root
`docker-compose -p bitcoin exec node bash`
As bitcoin
`docker-compose -p bitcoin exec -u bitcoin node bash`
Then: bitcoin-cli / bitcoin-tx available from within inside of container.

Note: if running as root, need to specify: -datadir=/home/bitcoin/.bitcoin

## Execute shell commands

`docker-compose -p bitcoin exec node ip a`

## Use bitcoin CLI from outside container

```
docker-compose -p bitcoin exec -u bitcoin node bitcoin-cli -rpcport=18332 \
    -rpcuser=username -rpcpassword=password getblockchaininfo
docker-compose -p bitcoin exec -u bitcoin node bitcoin-cli -rpcport=18332 \
    -rpcuser=username -rpcpassword=password getnetworkinfo
docker-compose -p bitcoin exec -u bitcoin node bitcoin-cli -rpcport=18332 \
    -rpcuser=username -rpcpassword=password getwalletinfo
```

## Stop

`docker-compose -p bitcoin stop`

## Start

`docker-compose -p bitcoin start`

## Remove stack

`docker-compose -p bitcoin rm -f`

## Modify startup parameters

### Edit: docker-compose.yml file

```
      bitcoind
        -printtoconsole
        -rpcuser=${BITCOIN_RPC_USER:-username}
        -rpcpassword=${BITCOIN_RPC_PASSWORD:-password}
        -testnet
```

## Use your configuration file

### Create bitcoin.conf
```
testnet=0
daemon=0
txindex=1
```

### Modify docker-compose.yml to
```
    volumes:
      - /home/bitcoin
      - ./bitcoin.conf:/bitcoin.conf
    command: >
      bitcoind
        -printtoconsole
        -rpcuser=${BITCOIN_RPC_USER:-username}
        -rpcpassword=${BITCOIN_RPC_PASSWORD:-password}
        -conf=/bitcoin.conf
```

### Start with

`docker-compose -p bitcoin up -d`

## Check container logs

```
docker-compose -p bitcoin logs

docker-compose -p bitcoin exec -u bitcoin node \
    tail -f /home/bitcoin/.bitcoin/testnet3/debug.log
```

## Scale containers

`docker-compose -p bitcoin scale node=2`

### Output
```
docker-compose -p bitcoin ps
     Name                   Command               State                                                  Ports                                                
--------------------------------------------------------------------------------------------------------------------------------------------------------------
bitcoin_node_1   /docker-entrypoint.sh bitc ...   Up      0.0.0.0:32833->18332/tcp, 0.0.0.0:32832->18333/tcp, 0.0.0.0:32835->8332/tcp, 0.0.0.0:32834->8333/tcp
bitcoin_node_2   /docker-entrypoint.sh bitc ...   Up      0.0.0.0:32837->18332/tcp, 0.0.0.0:32836->18333/tcp, 0.0.0.0:32839->8332/tcp, 0.0.0.0:32838->8333/tcp
```
