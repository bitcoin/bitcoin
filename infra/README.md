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
