// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETGLOBALS_H
#define BITCOIN_NETGLOBALS_H

#include <string>

// TODO: Get rid of these and delete this file.
//
// These values do not change after being set at startup. Instead of using
// globals, the values should be passed to the necessary subsystems as options.

extern bool fDiscover;
extern bool fListen;

/** Subversion as sent to the P2P network in `version` messages */
extern std::string strSubVersion;

#endif // BITCOIN_NETGLOBALS_H
