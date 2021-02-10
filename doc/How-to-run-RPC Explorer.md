# BTCE RPC Explorer

![image](./images/btce_rpc_explorer.PNG)



## Prerequisites

- `bitcoin-ecc` 가 작동 중인 상태여야 합니다.
- `npm` 을 설치해야합니다.

```shell
$ bitcoind -txindex -daemon
$ sudo apt-get install npm
```



## Installation

```shell
$ npm install -g btce-rpc-explorer
```



## Run server

**로컬 환경에서만 작동시키고 싶다면:**

```shell
$ btce-rpc-explorer --port 8080 --bitcoind-port 9776
```

웹브라우저에서 http://127.0.0.1:8080 를 입력하세요.


<hr>

**외부에서 접속하고 싶다면:**

```shell
$ btce-rpc-explorer --host 0.0.0.0 --port 8080 --bitcoind-port 9776
```

웹브라우저에서 http://본인아이피:8080 를 입력하세요.    