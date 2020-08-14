// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

/**
 * network protocol versioning
 */


static const int PROTOCOL_VERSION = 70218;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70213;

//! minimum proto version of masternode to accept in DKGs
static const int MIN_MASTERNODE_PROTO_VERSION = 70218;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! introduction of LLMQs
static const int LLMQS_PROTO_VERSION = 70214;

//! introduction of SENDDSQUEUE
//! TODO we can remove this in 0.15.0.0
static const int SENDDSQUEUE_PROTO_VERSION = 70214;

//! protocol version is included in MNAUTH starting with this version
static const int MNAUTH_NODE_VER_VERSION = 70218;

#endif // BITCOIN_VERSION_H
