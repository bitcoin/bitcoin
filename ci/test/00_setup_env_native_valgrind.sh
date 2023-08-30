#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export PACKAGES="valgrind clang llvm python3-zmq libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev"
export USE_VALGRIND=1
export NO_DEPENDS=1
if [[ "${TRAVIS}" == "true" && "${TRAVIS_REPO_SLUG}" != "bitcoin/bitcoin" ]]; then
  export TEST_RUNNER_EXTRA="wallet_disable"  # Only run wallet_disable as a smoke test to not hit the 50 min travis time limit
else
  export TEST_RUNNER_EXTRA="--exclude rpc_bind"  # Excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
fi
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --with-gui=no --enable-suppress-external-warnings CC=clang-15 CXX=clang++-15"  # TODO enable GUI
