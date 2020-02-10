# Bitcoin-Ecc

## What is the ECCPoW?

[Go to ECCPoW Repository](https://github.com/cryptoecc/ECCPoW)

## Build

**Install Xcode command line tool (for Mac)**

```bash
$ xcode-select -install

```

**Install Packages (for Mac)**

```bash
$ brew install automake berkeley-db4 libtool boost miniupnpc openssl pkg-config protobuf python qt libevent qrencode librsvg
```


**Install Packages (For Ubuntu)**
```bash
$ apt install -y build-essential libtool autotools-dev automake pkg-config bsdmainutils python3 ibssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libdb-dev libdb++-dev
```

**Clone SourceCode**

```bash
$ git clone https://github.com/cryptoecc/bitcoin_ECC.git
$ git checkout ecc-0.1
```

**Build BitcoinEcc**

```bash
$ cd bitcoin_ECC
$ ./autogen.sh
$ ./configure
$ make -j8
```

**Run**

```bash
src/bitcoind \
-rpcuser=<user> \
-rpcpassword=<password> \
-txindex \
-rpcallowip=0.0.0.0/0 \
-rpcbind=0.0.0.0
```


### Docker

#### Install Docker-CE

**Macos**

* [https://docs.docker.com/docker-for-mac/install/](https://docs.docker.com/docker-for-mac/install/) 
* Download: [https://download.docker.com/mac/stable/Docker.dmg](https://download.docker.com/mac/stable/Docker.dmg)

**Ubuntu**

* [Get Docker CE for Ubuntu | Docker Documentation](https://docs.docker.com/install/linux/docker-ce/ubuntu/)

#### Create Docker Image

**Clone SourceCode**

```bash
$ git clone https://github.com/cryptoecc/bitcoin_ECC.git
$ git checkout ecc-0.1
```

**Create Docker Image**

```bash
$ cd bitcoin_ECC
$ docker build -t bitcoin-ecc .
```

#### Run DockerContainer

**Createe Volume**

```bash
# docker volume rm bitcoin-ecc-data
$ docker volume create --name=bitcoin-ecc-data
```

**Mainnet**

```bash
$ docker run -d -v bitcoin-ecc-data:/root/.bitcoin --name=bitcoin-ecc-mainnet \
-p 9776:9776 \
-p 9777:9777 \
bitcoin-ecc \
-rpcuser=rpc \
-rpcpassword=rpc

$ docker logs bitcoin-ecc-mainnet
```

**Testnet**

```bash
$ docker run -d -v bitcoin-ecc-data:/root/.bitcoin --name=bitcoin-ecc-testnet \
-p 19776:19776 \
-p 19777:19777 \
bitcoin-ecc \
-rpcuser=rpc \
-rpcpassword=rpc \
-testnet

$ docker logs bitcoin-ecc-testnet
```

**Regtest**

```bash
$ docker run -d -v bitcoin-ecc-data:/root/.bitcoin --name=bitcoin-ecc-regtest \
-p 19887:19887 \
bitcoin-ecc \
-rpcuser=rpc \
-rpcpassword=rpc \
-regtest

$ docker logs bitcoin-ecc-regtest
```

**Add Node Option**

```
-addnode=<ipaddr>
```
