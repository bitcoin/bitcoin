# ITCoin demo

This document describes the steps to run an ITCoin node.

## Preliminaries

Make sure you have followed the preliminary `bitcoin-core` build instructions to install the required dependencies for your specific platform:

- [macOS](https://github.com/bitcoin/bitcoin/blob/master/doc/build-osx.md)
- [UNIX](https://github.com/bitcoin/bitcoin/blob/master/doc/build-unix.md)
- [Windows](https://github.com/bitcoin/bitcoin/blob/master/doc/build-windows.md)

Note: you also need to install the Berkeley DB and sqlite3 as ItCoin build needs to be built in wallet mode.

Then, to build ItCore and run the scripts, you need to install further dependencies (assuming macOS or GNU/Linux systems):

- Install GNU C++ compiler, version 10.
- Install `jq`
- Install `libboost-thread-dev`
- Install the `gettext` package (for the tool `envsubst`)

## Run an ITCoin miner

- From the `infra/` directory, run:
```
./configure-itcoin-core.sh && \
    make --jobs=$(nproc --ignore=1) --max-load=$(nproc --ignore=1) && \
    make install-strip
```
This command will build all the targets in the `target/` directory
in the repository root.

- Then:
```
./initialize-itcoin-local.sh
```
This step requires the `datadir` directory name to be available.
You should see an output similar to:
```
create-keypair: waiting (at most 10 seconds) for itcoin daemon to warmup
create-keypair: warmed up
create-keypair: cleaning up (deleting temporary directory /tmp/itcoin-temp-DT5b)
Creating datadir /home/default/infra/datadir. If it already exists this script will fail
Bitcoin Core starting
ItCoin daemon: waiting (at most 10 seconds) for warmup
ItCoin daemon: warmed up
Create wallet itcoin_signer
Wallet itcoin_signer created
Import private key into itcoin_signer
Private key imported into itcoin_signer
Generate an address
Address tb1qahalnde6t5293df6g0hhwfczf9y48gapjyz5x0 generated
Mine the first block
2021-09-28 14:09:58 INFO Using nbits=1e0377ae
2021-09-28 14:09:59 INFO Mined block at height 1; next in -1h59m1s (mine)
First block mined

To continue the mining process please run:

    ./continue-mining-local.sh
Bitcoin Core stopping
```

- Finally, to run an ITCoin miner node:
```
./continue-mining-local.sh
```
You should be able to see an output similar to the following:
```
Bitcoin Core starting
ItCoin daemon: waiting (at most 10 seconds) for warmup
ItCoin daemon: warmed up
Load wallet itcoin_signer
Wallet itcoin_signer loaded
Retrieve the address of the first transaction we find
Address tb1qahalnde6t5293df6g0hhwfczf9y48gapjyz5x0 retrieved
Keep mining eternally
2021-09-28 14:11:09 INFO Using nbits=1e0377ae
2021-09-28 14:11:09 INFO Mined block at height 2; next in -1h59m11s (mine)
2021-09-28 14:11:11 INFO Mined block at height 3; next in -1h58m13s (mine)
2021-09-28 14:11:12 INFO Mined block at height 4; next in -1h57m14s (mine)
2021-09-28 14:11:13 INFO Mined block at height 5; next in -1h56m15s (mine)
2021-09-28 14:11:14 INFO Mined block at height 6; next in -1h55m16s (mine)
2021-09-28 14:11:14 INFO Mined block at height 7; next in -1h54m16s (mine)
```

- In another terminal, go to `target/bin/` and to interact with the 
node, run:
```
./bitcoin-cli -signet -conf=../../infra/datadir/bitcoin.conf -rpcuser=user -rpcpassword=password <command>
```

For the list of commands, read [this documentation page](https://chainquery.com/bitcoin-cli).
