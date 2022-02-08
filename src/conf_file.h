// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONF_FILE_H
#define BITCOIN_CONF_FILE_H

static const char* DEFAULT_BITCOIN_CONF_TEXT = R"(##
## bitcoin.conf configuration file
##

# [network]
# Add a node IP address to connect to and attempt to keep the connection open. This option can be set multiple times.
#addnode=randomchars.onion
# Bind to given address and always listen on it. (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections
#bind=127.0.0.1:8333
# Allow DNS lookups for -addnode, -seednode and -connect values.
#dns=0
# Query for peer addresses via DNS lookup, if low on addresses.
#dnsseed=0
# Specify your own public IP address.
#externalip=your_tor_address.onion
# Accept incoming connections from peers.
#listen=0
# Use separate SOCKS5 proxy <ip:port> to reach peers via Tor hidden services.
#onion=127.0.0.1:9050
# Connect through <ip:port> SOCKS5 proxy.
#proxy=127.0.0.1:9050
# Connect to a node (IP address) to retrieve peer addresses, then disconnect.
#seednode=randomchars.onion


# [Sections]
# Most options apply to all networks.
# An option can be confined to one network by adding in the relevant section or adding network before option. Example: test.rpcport = 18332
# EXCEPTIONS: The options addnode, connect, port, bind, rpcport, rpcbind and wallet
# only apply to mainnet unless they appear in the appropriate section below.

# Options only for mainnet
#[main]

# Options only for testnet
#[test])";

#endif // BITCOIN_CONF_FILE_H
