This dir is only for internal e2e testing. 

##### Run node X
```bash
cd e2e/nodeX
docker-compose up -d
# go back to top level dir
cd ../../ 
# run daemon
./src/bitcoind -datadir=e2e/nodeX/btc
# run cli
./src/bitcoin-cli -datadir=e2e/nodeX/btc <commands>
```

```
bitcoind rpc user : hello
bitcoind rpc pass : A1234567890!a
bitcoind rpc      : 1800X
alt port          : 1900X
bitcoind p2p      : 2000X 
```
