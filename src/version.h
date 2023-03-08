// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_VERSION_H
#define SYSCOIN_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 70017;

//! Version when we switched to a size-based "headers" limit.
static const int SIZE_HEADERS_LIMIT_VERSION = 70015;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 31800;

// SYSCOIN
//! minimum proto version of masternode to accept in DKGs
static const int MIN_MASTERNODE_PROTO_VERSION = 70017;

//! protocol version is included in MNAUTH starting with this version
static const int MNAUTH_NODE_VER_VERSION = 70016;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
static const int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
static const int INVALID_CB_NO_BAN_VERSION = 70015;

//! "wtxidrelay" command for wtxid-based relay starts with this version
static const int WTXID_RELAY_VERSION = 70016;

//! BLS scheme was introduced in this version
static const int BLS_SCHEME_PROTO_VERSION = 70017;
// Make sure that none of the values above collide with
// `SERIALIZE_TRANSACTION_NO_WITNESS` or `ADDRV2_FORMAT`.

#endif // SYSCOIN_VERSION_H
