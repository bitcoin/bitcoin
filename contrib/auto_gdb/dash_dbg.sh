#!/usr/bin/env bash
# Copyright (c) 2018-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# use testnet settings,  if you need mainnet,  use ~/.dashcore/dashd.pid file instead
export LC_ALL=C

dash_pid=$(<~/.dashcore/testnet3/dashd.pid)
sudo gdb -batch -ex "source debug.gdb" dashd ${dash_pid}
