// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

/**
 * network protocol versioning
 */


static const int PROTOCOL_VERSION = 70231;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70215;

//! minimum proto version of masternode to accept in DKGs
static const int MIN_MASTERNODE_PROTO_VERSION = 70230;

//! protocol version is included in MNAUTH starting with this version
static const int MNAUTH_NODE_VER_VERSION = 70218;

//! introduction of QGETDATA/QDATA messages
static const int LLMQ_DATA_MESSAGES_VERSION = 70219;

//! introduction of instant send deterministic lock (ISDLOCK)
static const int ISDLOCK_PROTO_VERSION = 70220;

//! GOVSCRIPT was activated in this version
static const int GOVSCRIPT_PROTO_VERSION = 70221;

//! ADDRV2 was introduced in this version
static const int ADDRV2_PROTO_VERSION = 70223;

//! BLS scheme was introduced in this version
static const int BLS_SCHEME_PROTO_VERSION = 70225;

//! Masternode type was introduced in this version
static const int DMN_TYPE_PROTO_VERSION = 70227;

//! Versioned Simplified Masternode List Entries were introduced in this version
static const int SMNLE_VERSIONED_PROTO_VERSION = 70228;

//! Versioned Simplified Masternode List Entries were introduced in this version
static const int MNLISTDIFF_VERSION_ORDER = 70229;

//! Masternode type was introduced in this version
static const int MNLISTDIFF_CHAINLOCKS_PROTO_VERSION = 70230;

//! Legacy ISLOCK messages and a corresponding INV were dropped in this version
static const int NO_LEGACY_ISLOCK_PROTO_VERSION = 70231;

// Make sure that none of the values above collide with `ADDRV2_FORMAT`.

#endif // BITCOIN_VERSION_H
