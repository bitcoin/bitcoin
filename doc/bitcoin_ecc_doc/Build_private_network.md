# Build private network and test (BTC-ECC)

Writer: Hyunjun Jung, junghj85@gist.ac.kr, Ha-young Park (mintyoung@gm.gist.ac.kr)

Contact PI: Proof. Heung-No Lee, heungno@gist.ac.kr

Github for this example: https://github.com/cryptoecc/bitcoin_ECC

For more information: [INFONET](https://infonet.gist.ac.kr/)

BTC-ECC is a new bitcoin core program. BTC-ECC replaced bitcoin consensus with ECCPoW.

Today we will build our own private network and test it is working well. However, we will use only 1 node for today. We will try a multi-node example later on.

## VMWare ECCPoW Ubuntu 
Plz. Use the vmware file temporarily. (2G ram ver)

file1 : https://drive.google.com/file/d/1-lsbqV8R03m7cjPb-Om4ietIIo2uOiO-/view?usp=sharing (password: wjdguswns)

## 1. Environment

The BTC-ECC package works made the following environment.

- OS: Ubuntu 18.04 LTS (https://releases.ubuntu.com/18.04/)
- Git location of the package:  (https://github.com/cryptoecc/bitcoin_ECC, ver0.1.2)



The following 8 steps are to download, install, and run BTC-ECC daemon on your computer.



### 1.1 Install of build tool

```bash
$ sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils
```

```bash
$ sudo apt-get install libqrencode-dev autoconf openssl libssl-dev libevent-dev libminiupnpc-dev
```



### 1.2 Install of boost labrary

```bash
$ sudo apt-get install libboost-all-dev
```



### 1.3 Install of berkeley DB

```bash
$ sudo apt-get install software-properties-common
$ sudo add-apt-repository ppa:bitcoin/bitcoin
$ sudo apt-get update
$ sudo apt-get install libdb4.8-dev libdb4.8++-dev
```



### 1.4 Install of qt-wallet

```bash
$ sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev
```



### 1.5 ECCPoW blockchain code download and version change

```bash
$ git clone https://github.com/cryptoecc/bitcoin_ECC.git
$ cd bitcoin_ECC
$ git checkout ecc-0.1.2
```



### 1.6 Source code build

```bash
$ cd bitcoin_ECC
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
```

If you get an error when executing `./configure` then try this.

```bash
$ sudo apt-get install libdb++-dev 
```



### 1.7 Execute bitcoin core

```bash
$ bitcoind -txindex -daemon
```



## 2. BTC-ECC Parameters

The BTC-ECC can be configured to build Mainnet, Testnet, and Regtest networks. You can set up a private network using the CMainParams, CTestNetParams, CRegTestParams classes defined in "chainparams.cpp". We will test the private network using the Mainnet option in this example.



The `chainparams.cpp` source code is as follows:

```
class CMainParams : public CChainParams {
public:
  CMainParams() {
	 (.....)
  consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffff
ffffffffffffffff");
  consensus.nPowTargetTimespan = 60 * 60;
  consensus.nPowTargetSpacing = 1 * 60;
  consensus.fPowAllowMinDifficultyBlocks = true;
  consensus.fPowNoRetargeting = true;
    (.....)
  nDefaultPort = 9777; 
  int init_level = 10;
  genesis = CreateGenesisBlock(1558627231, 399, init_level, 1, 50 *     COIN); 
  assert(consensus.hashGenesisBlock == uint256S("b77abb03a0a8a4f23a73
80bf655af8312c4769c64fcbf335a08d598b13368f22")); 
  assert(genesis.hashMerkleRoot == uint256S("15d2f927fe3eafe88ce0b4ccf2
67727ed306295051339a16e0b95067e65bead8"));
    (.....)
  vSeeds.emplace_back("...");
    (.....)
  m_fallback_fee_enabled = false;
  }
};
```

| Parameters                   | description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| powLimit                     | Minimum difficulty setting                                   |
| nPowTargetTimespan           | Difficulty level change cycle (currently 60 minutes)         |
| nPowTargetSpacing            | Block generation cycle (currently 1 minute)                  |
| fPowAllowMinDifficultyBlocks | Permission below minimum difficulty level                    |
| fPowNoRetargeting            | Permission to change difficulty level                        |
| nDefaultPort                 | ECCPoW Blockchain port number (currently port 9777)          |
| init_level                   | ECCPoW difficulty level (currently difficulty level 10)      |
| CreateGenesisBlock           | Linux time, Nonce, nBit, nVersion, compensation amount entered in order to generation the Genesis block |
| vSeeds.emplace_back          | Connection seed address                                      |
| m_fallback_fee_enabled       | Permission of coin transfer fee setting                      |





## 3. Test Private Network

We aim to BTC-ECC Mainnet in the local network.

Open a new terminal under `/bitcoin_ECC`  directory. Do the following procedure in the terminal.



### 3.1 Execute BTC-ECC 

```shell
$ bitcoind -txindex -daemon

mainnet with level = 1
set is constructed from 10 to 22 with step 2
n : 32	 wc : 3	 wr : 4
mainnet n: 399 Hash: 0ccc07b781737bf3c901307dd92ed0548215b745b38daee5fdbb4f042dc4885c
Bitcoin server starting
```



### 3.2 Get blockcount

```shell
$ bitcoin-cli getblockcount

0
```

The command `getblockcount` checks the number of blocks. We have just generated this network. There is no block generated yet. 



### 3.3 Generate an account

```shell
$ bitcoin-cli getnewaddress

39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M
```

We have just generated the new address, `39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M`. 



### 3.4 Generate ten new blocks

```shell
$ bitcoin-cli generatetoaddress 10 9naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M

[
  "fa2549dbcbecfbaff696abd19e086b82e7288cc3c726b6fc4cfc3754a4200a31",
  "09df0e57e1a4f84c594a5553dd2c3cc01c380ba92bf37268a84fbad62ed79693",
  "abc2bce6e224b68da443b505d779cf91715119847d457068c47fa0529dbf71a4",
  "951ae98c7193e11f7469245ec2f18a20103e8e26afe3d7eef10e7a22d15ed2dd",
  "c55a5294d3f8b0c3259cf3842ad0ad9bbb3309523938919dbd59cd1cb14b530a",
  "b2d7b23c8c3ba403b05f6111f49c65212eba0ca61378e15612c04269e3177a91",
  "c4ea7cfc8cafe7e9e8989a5c5808d3e01e83794ebd56278ea9013571e480791d",
  "ac8eb56da73ee9bce603cdda83b1a89903e6cd6828ea5f9c6ab0c8572a780f4b",
  "bd1f46d19d6db8c35536594e6a39339c15ef0185bd244afe7c0af022e6ebf29e",
  "5d4cc21450bcb8df125b8a5301e80f2bc35151ffd1d47a65999238073733507e"
]
```



### 3.5 Check blockchain information

```shell
$ bitcoin-cli getblockchaininfo

{
  "chain": "main",
  "blocks": 10,
  "headers": 10,
  "bestblockhash": "5d4cc21450bcb8df125b8a5301e80f2bc35151ffd1d47a65999238073733507e",
  "difficulty": 4.523059468369196e+74,
  "mediantime": 1590478786,
  "verificationprogress": 1,
  "initialblockdownload": false,
  "chainwork": "00000000000000000000000000000000000000000000000000000000000573f8",
  "size_on_disk": 3285,
  "pruned": false,
  "softforks": [
    {
      "id": "bip34",
      "version": 2,
      "reject": {
        "status": false
      }
    },
    {
      "id": "bip66",
      "version": 3,
      "reject": {
        "status": false
      }
    },
    {
      "id": "bip65",
      "version": 4,
      "reject": {
        "status": false
      }
    }
  ],
  "bip9_softforks": {
    "csv": {
      "status": "defined",
      "startTime": 1462060800,
      "timeout": 1493596800,
      "since": 0
    },
    "segwit": {
      "status": "active",
      "startTime": -1,
      "timeout": 9223372036854775807,
      "since": 0
    }
  },
  "warnings": ""
}

```

```shell
$ bitcoin-cli getbalance
450.00000000
```



## 4. Send Transaction

We want Alice to send Bob a 0.025 BTC-BTC. 

We use are the commands  

1) `listupspent` 2) `createrawtransaction` 3) `signrawtransaction` 4) `sendrawtransaction`.



### Given:

- Alice is the payer. Alice address : `39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M`
- Bob is the payee. Bob address : `345g89pujpmSSFuCWeizqbvVe92pRWZarB`
- Alice sends to Bob : 0.025 BTC-BTC



### 4.1 listunspent

Show confirmed outputs(unspend) in Alice address  `39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M`.

```shell
$ bitcoin-cli listunspent 0 99999 '["39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M"]'

[
  {
    "txid":"c1cd952807c0e0b2be340a0413fe009a72bfe2ab52e447f2c5b88e033ddfc444",
    "vout":0,
    "address":"39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M",
    "label":"",
    "redeemScript":"0014869c8d0d64aa4aba5a8336821384b3c8e8631026",
    "scriptPubKey":"a91458ce290b131d50345f67f0c330d900be55c5e8f587",
    "amount":50.00000000,
    "confirmations":7,
    "spendable":true,
    "solvable":true,
    "desc":"sh(wpkh([39d8d30c/0'/0'/0']03290574714e059efbb6248b7058901639932d4d32a6f3682cf21f0dd74bf4bf41))#4s4zcv6g",
    "safe":true
  },
  ...
```



### 4.2 createrawtransaction

```shell
$ bitcoin-cli createrawtransaction '[{"txid":"c1cd952807c0e0b2be340a0413fe009a72bfe2ab52e447f2c5b88e033ddfc444","vout":0}]' '{"345g89pujpmSSFuCWeizqbvVe92pRWZarB":0.025, "39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M":49.9745}'

020000000144c4df3d038eb8c5f247e452abe2bf729a00fe13040a34beb2e0c0072895cdc10000000000ffffffff02a02526000000000017a9141a394a65ad9ebfc907bac968a4e86cbb164c9c85871009df290100000017a91458ce290b131d50345f67f0c330d900be55c5e8f58700000000
```

The input field includes 

```shell
1) Txid: `c1cd952807c0e0b2be340a0413fe009a72bfe2ab52e447f2c5b88e033ddfc444`

2) Recipient address :  `39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M`

3) Amount to send : 0.025

4) Sender address : `345g89pujpmSSFuCWeizqbvVe92pRWZarB`

5) Amount change : 49.9745
```



### 4.3 signrawtransaction

Check the private key of `39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M`.

```shell
$ bitcoin-cli dumpprivkey 39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M

L3n5FQiuBGtNAC47RPCKGNiADwNfeyodKfbP5zLWgd8aJKEbF2ey
```

Signs the transaction in the serialized transaction format using private keys.

```shell
$ bitcoin-cli signrawtransactionwithkey "020000000144c4df3d038eb8c5f247e452abe2bf729a00fe13040a34beb2e0c0072895cdc10000000000ffffffff02a02526000000000017a9141a394a65ad9ebfc907bac968a4e86cbb164c9c85871009df290100000017a91458ce290b131d50345f67f0c330d900be55c5e8f58700000000" '["L3n5FQiuBGtNAC47RPCKGNiADwNfeyodKfbP5zLWgd8aJKEbF2ey"]'

{
  "hex":"0200000000010144c4df3d038eb8c5f247e452abe2bf729a00fe13040a34beb2e0c0072895cdc10000000017160014869c8d0d64aa4aba5a8336821384b3c8e8631026ffffffff02a02526000000000017a9141a394a65ad9ebfc907bac968a4e86cbb164c9c85871009df290100000017a91458ce290b131d50345f67f0c330d900be55c5e8f58702473044022075b8645993714ba61c63a177437c3146145a9daebf25f93324eadf767e0277cd02202648a4817686bdddb57944c80e9a0a3a4daeb8defd59d4fc1d118fd814392909012103290574714e059efbb6248b7058901639932d4d32a6f3682cf21f0dd74bf4bf4100000000",
"complete":true
}
```

The command `signrawtransaction`  returns another hex-encoded raw transaction.



### 4.4 sendrawtransaction

```shell
$ bitcoin-cli sendrawtransaction 0200000000010144c4df3d038eb8c5f247e452abe2bf729a00fe13040a34beb2e0c0072895cdc10000000017160014869c8d0d64aa4aba5a8336821384b3c8e8631026ffffffff02a02526000000000017a9141a394a65ad9ebfc907bac968a4e86cbb164c9c85871009df290100000017a91458ce290b131d50345f67f0c330d900be55c5e8f58702473044022075b8645993714ba61c63a177437c3146145a9daebf25f93324eadf767e0277cd02202648a4817686bdddb57944c80e9a0a3a4daeb8defd59d4fc1d118fd814392909012103290574714e059efbb6248b7058901639932d4d32a6f3682cf21f0dd74bf4bf4100000000

f2fc88a8bc534cfb7e6bc8e7c7d944f440a6612a8b7d34c166ca76765e168c56
```

The command `sendrawtransaction` returns a transaction hash (txid) as it submits the transaction on the network.



### 4.5 Verification of the issued transaction in a block

Forming a new block with the created Tx included.



Generate 1 block.

```shell
$ bitcoin-cli generatetoaddress 1 39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M

[
  "f1d741990690527423d076fc77e8f26d26dc12b90251f912fcb3332253bdb615"
]
```

Get block information.

```shell
$ bitcoin-cli getblock f1d741990690527423d076fc77e8f26d26dc12b90251f912fcb3332253bdb615

{
  "hash":"f1d741990690527423d076fc77e8f26d26dc12b90251f912fcb3332253bdb615",  
  "confirmations":1,
  "strippedsize":352,
  "size":497,
  "weight":1553,
  "height":11,
  "version":536870912,
  "versionHex":"20000000",
  "merkleroot":"1a9966b59abc63c6485eafa2f466a22e042d76c1b0718263a5c21bcd9424ac10",
  "tx":[
    "f983365fe16cf253221917ede164f9c15476695cc5fad141fec3c6705c00d436",
    "f2fc88a8bc534cfb7e6bc8e7c7d944f440a6612a8b7d34c166ca76765e168c56"
  ],
  "time":1591687898,
  "mediantime":1590478786,
  "nonce":6,
  "bits":"00000001",
  "difficulty":4.523059468369196e+74,
  "chainwork":"000000000000000000000000000000000000000000000000000000000005f2e0",
  "nTx":2,
  "previousblockhash":"5d4cc21450bcb8df125b8a5301e80f2bc35151ffd1d47a65999238073733507e"
}
```

Get transaction information.

```shell
$ bitcoin-cli gettransaction f2fc88a8bc534cfb7e6bc8e7c7d944f440a6612a8b7d34c166ca76765e168c56

{
  "amount":0.00000000,
  "fee":-0.00050000,
  "confirmations":1,
  "blockhash":"f1d741990690527423d076fc77e8f26d26dc12b90251f912fcb3332253bdb615",
  "blockindex":1,
  "blocktime":1591687898,
  "txid":"f2fc88a8bc534cfb7e6bc8e7c7d944f440a6612a8b7d34c166ca76765e168c56",
  "walletconflicts":[
  ],
  "time":1591687164,
  "timereceived":1591687164,
  "bip125-replaceable":"no",
  "details":[
    {
      "address":"345g89pujpmSSFuCWeizqbvVe92pRWZarB",
      "category":"send",
      "amount":-0.02500000,
      "label":"",
      "vout":0,
      "fee":-0.00050000,
      "abandoned":false
    },
    {
      "address":"39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M",
      "category":"send",
      "amount":-49.97450000,
      "label":"",
      "vout":1,
      "fee":-0.00050000,
      "abandoned":false
    },
    {
      "address":"345g89pujpmSSFuCWeizqbvVe92pRWZarB",
      "category":"receive",
      "amount":0.02500000,
      "label":"",
      "vout":0
    },
    {
      "address":"39naMhHQgXwm7CVdEaL4PGUkp4fUdHX14M",
      "category":"receive",
      "amount":49.97450000,
      "label":"",
      "vout":1
    }
  ],
  "hex":"0200000000010144c4df3d038eb8c5f247e452abe2bf729a00fe13040a34beb2e0c0072895cdc10000000017160014869c8d0d64aa4aba5a8336821384b3c8e8631026ffffffff02a02526000000000017a9141a394a65ad9ebfc907bac968a4e86cbb164c9c85871009df290100000017a91458ce290b131d50345f67f0c330d900be55c5e8f58702473044022075b8645993714ba61c63a177437c3146145a9daebf25f93324eadf767e0277cd02202648a4817686bdddb57944c80e9a0a3a4daeb8defd59d4fc1d118fd814392909012103290574714e059efbb6248b7058901639932d4d32a6f3682cf21f0dd74bf4bf4100000000"
}
```

Please Note. There are no leading zeros in the block hash because ECCPoW is used.


### 4.6 Connection node



```shell
$ bitcoin-cli addnode <ip_address> <add / remove / onetry>
```

Check connected node.

```shell
$ bitcoin-cli getpeerinfo
```


End.
