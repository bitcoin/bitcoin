// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_MESSAGE_H
#define BITCOIN_UTIL_MESSAGE_H

#include <uint256.h>

#include <string>

/**
 * Hashes a message for signing and verification in a manner that prevents
 * inadvertently signing a transaction.
 */
uint256 HashMessage(const std::string& message);

#endif // BITCOIN_UTIL_MESSAGE_H
