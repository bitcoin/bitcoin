// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

/**
 * network protocol versioning
 */


static const int PROTOCOL_VERSION = 70213;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 70077;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 70210;

//! disconnect from peers older than this proto version when DIP3 is activated via the BIP9 deployment
static const int MIN_PEER_PROTO_VERSION_DIP3 = 70213;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "mempool" command, enhanced "getdata" behavior starts with this version
static const int MEMPOOL_GD_VERSION = 60002;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70201;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70201;

//! DIP0001 was activated in this version
static const int DIP0001_PROTOCOL_VERSION = 70208;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70209;

//! introduction of DIP3/deterministic masternodes
static const int DMN_PROTO_VERSION = 70213;

#endif // BITCOIN_VERSION_H
