// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 70201;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 70077;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70103;

//! minimum peer version accepted by DarksendPool
static const int MIN_POOL_PEER_PROTO_VERSION = 70201;

//! minimum peer version for masternode budgets
static const int MSG_GOVERNANCE_PEER_PROTO_VERSION = 70201;

//! minimum peer version for masternode winner broadcasts
static const int MIN_MNW_PEER_PROTO_VERSION = 70103;

//! minimum peer version that can receive masternode payments
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1 = 70103;
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2 = 70201;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 32000;
static const int NOBLKS_VERSION_END = 32400;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "mempool" command, enhanced "getdata" behavior starts with this version
static const int MEMPOOL_GD_VERSION = 60002;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70012;

#endif // BITCOIN_VERSION_H
