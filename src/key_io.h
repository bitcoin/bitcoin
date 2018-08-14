// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEY_IO_H
#define BITCOIN_KEY_IO_H

#include <attributes.h>
#include <chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <script/standard.h>

#include <string>

CKey DecodeSecret(const std::string& str);
std::string EncodeSecret(const CKey& key);

//! Returns true if and only if extkey is valid (extkey.key.IsValid()) when the function returns.
NODISCARD bool DecodeExtKey(const std::string& str, CExtKey& extkey);
std::string EncodeExtKey(const CExtKey& extkey);
//! Returns true if and only if extpubkey is valid (extpubkey.pubkey.IsValid()) when the function returns.
NODISCARD bool DecodeExtPubKey(const std::string& str, CExtPubKey& extpubkey);
std::string EncodeExtPubKey(const CExtPubKey& extpubkey);

std::string EncodeDestination(const CTxDestination& dest);
CTxDestination DecodeDestination(const std::string& str);
bool IsValidDestinationString(const std::string& str);
bool IsValidDestinationString(const std::string& str, const CChainParams& params);

#endif // BITCOIN_KEY_IO_H
