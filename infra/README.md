# Bitcoin infrastructure utilities

## Prerequisites

To use the utilities in this folder, you have to install the Boost library in `itcoin-core/../itcoin-pbft/usrlocal`, and the following procedure is provided as an example.

```bash
cd ~
wget https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.gz
tar -xvf boost_1_75_0.tar.gz
cd ~/boost_1_75_0
./bootstrap.sh --prefix=/home/ubuntu/itcoin-pbft/usrlocal
./b2 install
```

Additional itcoin-core dependencies can be installed from official repo in Ubuntu 20.04 LTS

```bash
sudo apt install \
    autoconf \
    automake \
    bsdmainutils \
    build-essential \
    g++-10 \
    gcc-10 \
    jq \
    libdb5.3++-dev \
    libevent-dev \
    libqrencode-dev \
    libqt5core5a \
    libqt5dbus5 \
    libqt5gui5 \
    libsqlite3-dev \
    libtool \
    libzmqpp-dev \
    pkg-config \
    python3 \
    python3-zmq \
    qttools5-dev \
    qttools5-dev-tools
```

## Build and install itcoin

```bash
cd ~
git clone git@github.com:bancaditalia/itcoin-core.git

cd ~/itcoin-core/infra
./configure-itcoin-core-dev.sh

make --jobs=$(nproc --ignore=1)
make install-strip
```

## Build a test network

```bash
cd ~/itcoin-core/infra

# Create a new 1-of-1 network
rm -rf datadir
./initialize-itcoin-local.sh 

# Continue mine the new network
./continue-mining-local.sh
```

## Running the itcoin-core tests with custom boost libraries

Run all tests with:

```bash
LD_LIBRARY_PATH=/home/ubuntu/itcoin-pbft/usrlocal/lib make check
LD_LIBRARY_PATH=/home/ubuntu/itcoin-pbft/usrlocal/lib test/functional/test_runner.py > $HOME/itcoin-draft-notes/`date +%Y%m%d%H%M%S`_test_runner_py_results.log 2>&1
```

Or run single tests with:

```
LD_LIBRARY_PATH=/home/ubuntu/itcoin-pbft/usrlocal/lib test/functional/wallet_send.py
```
