// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PROTOCOL_VERSION_H
#define BITCOIN_NODE_PROTOCOL_VERSION_H

/**
 * network protocol versioning
 */

inline constexpr int PROTOCOL_VERSION = 70017;

//! initial proto version, to be increased after version/verack negotiation
inline constexpr int INIT_PROTO_VERSION = 209;

//! disconnect from peers older than this proto version
inline constexpr int MIN_PEER_PROTO_VERSION = 31800;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
inline constexpr int BIP0031_VERSION = 60000;

//! "sendheaders" message type and announcing blocks with headers starts with this version
inline constexpr int SENDHEADERS_VERSION = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
inline constexpr int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
inline constexpr int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
inline constexpr int INVALID_CB_NO_BAN_VERSION = 70015;

//! "wtxidrelay" message type for wtxid-based relay starts with this version
inline constexpr int WTXID_RELAY_VERSION = 70016;

//! "feature" message type for feature negotiation starts with this version
inline constexpr int FEATURE_VERSION = 70017;

#endif // BITCOIN_NODE_PROTOCOL_VERSION_H
