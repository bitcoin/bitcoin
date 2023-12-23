// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEY_IO_H
#define BITCOIN_KEY_IO_H

#include <bech32_mod.h>
#include <blsct/double_public_key.h>
#include <chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <script/standard.h>

#include <string>

CKey DecodeSecret(const std::string& str);
std::string EncodeSecret(const CKey& key);

CExtKey DecodeExtKey(const std::string& str);
std::string EncodeExtKey(const CExtKey& extkey);
CExtPubKey DecodeExtPubKey(const std::string& str);
std::string EncodeExtPubKey(const CExtPubKey& extpubkey);

std::string EncodeDestination(const CTxDestination& dest);
CTxDestination DecodeDestination(const std::string& str);
CTxDestination DecodeDestination(const std::string& str, std::string& error_msg, std::vector<int>* error_locations = nullptr);
bool IsValidDestinationString(const std::string& str);
bool IsValidDestinationString(const std::string& str, const CChainParams& params);

// double public key after encoding to bech32_mod is 165-byte long consisting of:
// - 2-byte hrp
// - 1-byte separator '1'
// - 154-byte data
// - 8-byte checksum
constexpr size_t DOUBLE_PUBKEY_ENC_SIZE = 2 + 1 + bech32_mod::DOUBLE_PUBKEY_DATA_ENC_SIZE + 8;

/** Encode DoublePublicKey to Bech32 or Bech32m string. Encoding must be one of BECH32 or BECH32M. */
std::string EncodeDoublePublicKey(
    const CChainParams& params,
    const bech32_mod::Encoding encoding,
    const blsct::DoublePublicKey& dpk
);

/** Decode a Bech32 or Bech32m string to a DoublePublicKey. */
std::optional<blsct::DoublePublicKey> DecodeDoublePublicKey(
    const CChainParams& params,
    const std::string& str
);

#endif // BITCOIN_KEY_IO_H
