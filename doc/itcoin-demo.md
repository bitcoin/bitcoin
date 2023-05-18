# Itcoin-core demo

This document describes the steps to run a standalone itcoin-core node.


## Preliminaries

Itcoin can be run on a variety of unix-like systems. For simplicity's sake this
document assumes the use of an **Ubuntu 22.04 LTS** linux distribution.

Install the build toolchain:

```
sudo apt install --no-install-recommends -y \
    autoconf \
    automake \
    bsdextrautils \
    build-essential \
    g++ \
    libtool \
    pkg-config
```

Install the build and runtime dependencies:

```
sudo apt install --no-install-recommends -y \
    gettext-base \
    jq \
    libboost1.74-dev \
    libboost-filesystem1.74-dev \
    libboost-thread1.74-dev \
    libdb5.3++-dev \
    libevent-dev \
    libsqlite3-dev \
    libzmq3-dev \
    python3
```

For a more structured reference, you can also have a look at the [Dockerfile](/Dockerfile).


## Build itcoin-core

From the `infra/` directory, run:
```
./configure-itcoin-core-dev.sh && \
    make --jobs=$(nproc --ignore=1) --max-load=$(nproc --ignore=1) && \
    make install-strip
```

This command will build all the targets in the `target/` directory in the
repository root.


## Run an itcoin miner

These instructions assume that you have already built the project and that you
are in the `<base>/infra` directory.

```
rm -rf ./datadir
./initialize-itcoin-local.sh
```

You should see an output similar to:

```
create-initdata: waiting (at most 10 seconds) for itcoin daemon to warmup
create-initdata: warmed up
create-initdata: cleaning up (deleting temporary directory /tmp/itcoin-temp-wle0)
Creating datadir <base>/infra/datadir. If it already exists this script will fail
Itcoin Core starting
Itcoin daemon: waiting (at most 10 seconds) for warmup
Itcoin daemon: warmed up
Create a blank descriptor wallet itcoin_signer
Wallet itcoin_signer created
Import private descriptors into itcoin_signer
Private descriptors imported into itcoin_signer
Generate an address
Address tb1pl9q73nvvmyydhjct2w3z2esrjk82n7dxhpamwkumm7lr6fpkk08sl6vtrk generated
Mine the first block
2023-04-26 16:12:29 INFO Using nbits=207fffff
2023-04-26 16:12:29 INFO Mined block at height 1; next in -9m0s (mine)
First block mined

To continue the mining process please run:

    ./continue-mining-local.sh
Itcoin Core stopping
```

The result of this command is the creation of a `<base>/infra/datadir`, the
initialization of an itcoin network and the creation of its first block, with a
timestamp 10 minutes in the past.

Finally, to run a long running itcoin miner node:

```
./continue-mining-local.sh
```

You should be able to see an output similar to the following:

```
Itcoin Core starting
Itcoin daemon: waiting (at most 10 seconds) for warmup
Itcoin daemon: warmed up
Load wallet itcoin_signer
Wallet itcoin_signer loaded
Retrieve the address of the first transaction we find
Address tb1pl9q73nvvmyydhjct2w3z2esrjk82n7dxhpamwkumm7lr6fpkk08sl6vtrk retrieved
Keep mining eternally
2023-04-26 16:12:54 INFO Using nbits=207fffff
2023-04-26 16:12:54 INFO Mined block at height 2; next in -8m25s (mine)
2023-04-26 16:12:54 INFO Mined block at height 3; next in -7m25s (mine)
2023-04-26 16:12:54 INFO Mined block at height 4; next in -6m25s (mine)
2023-04-26 16:12:55 INFO Mined block at height 5; next in -5m26s (mine)
2023-04-26 16:12:55 INFO Mined block at height 6; next in -4m26s (mine)
2023-04-26 16:12:55 INFO Mined block at height 7; next in -3m26s (mine)
2023-04-26 16:12:56 INFO Mined block at height 8; next in -2m27s (mine)
2023-04-26 16:12:56 INFO Mined block at height 9; next in -1m27s (mine)
2023-04-26 16:12:56 INFO Mined block at height 10; next in -27s (mine)
2023-04-26 16:12:57 INFO Mined block at height 11; next in 32s (mine)
[...]
```

To interact with the node you can use the `bitcoin-cli` executable. Open another
terminal, go to `<base>/infra/` and run:

```
../target/bin/bitcoin-cli -datadir=datadir <command>
```

For the list of commands, read [this documentation page](https://chainquery.com/bitcoin-cli).
