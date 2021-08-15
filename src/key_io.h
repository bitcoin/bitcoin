// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEY_IO_H
#define BITCOIN_KEY_IO_H

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
bool IsValidDestinationString(const std::string& str);
bool IsValidDestinationString(const std::string& str, const CChainParams& params);

/**
 * Wrapper class for every supported address
 */
struct Destination {
public:
    explicit Destination() {}
    explicit Destination(const CTxDestination& _dest, bool _isP2CS) : dest(_dest), isP2CS(_isP2CS) {}

    CTxDestination dest{CNoDestination()};
    bool isP2CS{false};

    Destination& operator=(const Destination& from)
    {
        this->dest = from.dest;
        this->isP2CS = from.isP2CS;
        return *this;
    }

    std::string ToString()
    {
        if (!IsValidDestination(dest)) {
            // Invalid address
            return "";
        }
        return EncodeDestination(dest);
    }
};

#endif // BITCOIN_KEY_IO_H
