#!/usr/bin/env bash
#
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

# script for generating an example bitcoin.conf file from bitcoind -h
export LC_ALL=C

EXAMPLE_CONF_FILE=${1:-share/examples/bitcoin.conf}

echo 'Generating example bitcoin.conf file in share/examples/'
# create examples directory
mkdir -p share/examples

# create the header text
cat > ${EXAMPLE_CONF_FILE} << 'EOF'
##
## bitcoin.conf configuration file. Lines beginning with # are comments.
## All possible configuration options are provided. To use, copy this file
## to your data directory (default or specified by -datadir), uncomment and
## edit options you would like to change, and save the file.
##

EOF

# parse the output from bitcoind --help
bitcoind --help | sed '1,/Print this help message and exit/d' \
	| sed -E 's/^[[:upper:]]/# &/' \
	| sed -E 's/^[[:space:]]{2}\-/#/' \
	| sed -E 's/^[[:space:]]{7}/# /' \
	| sed -E '/[=[:space:]]/!s/#.*$/&=1/' \
	| awk '/^#[a-z]/{x=$0;next}{if (NF==0) print x"\n";else print}' >> ${EXAMPLE_CONF_FILE}

# create the footer text
cat >> ${EXAMPLE_CONF_FILE} << 'EOF'
# [Sections]
# Most options apply to mainnet, testnet, signet and regtest.
# If you want to confine an option to just one network, you should add it in the
# relevant section below.
# EXCEPTIONS: The options addnode, connect, port, bind, rpcport, rpcbind and wallet
# only apply to mainnet unless they appear in the appropriate section below.

# Options only for mainnet
[main]

# Options only for testnet
[test]

# Options only for signet
[signet]
EOF
