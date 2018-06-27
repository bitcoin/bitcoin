// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSIGHT_INSIGHT_H
#define BITCOIN_INSIGHT_INSIGHT_H

#include <uint256.h>
#include <amount.h>
#include <script/script.h>

#include <insight/addressindex.h>
#include <insight/spentindex.h>
#include <insight/timestampindex.h>

class CTxOutBase;

bool ExtractIndexInfo(const CScript *pScript, int &scriptType, std::vector<uint8_t> &hashBytes);
bool ExtractIndexInfo(const CTxOutBase *out, int &scriptType, std::vector<uint8_t> &hashBytes, CAmount &nValue, const CScript *&pScript);

/** Functions for insight block explorer */
bool GetTimestampIndex(const unsigned int &high, const unsigned int &low, const bool fActiveOnly, std::vector<std::pair<uint256, unsigned int> > &hashes);
bool GetSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value);
bool HashOnchainActive(const uint256 &hash);
bool GetAddressIndex(uint256 addressHash, int type,
                     std::vector<std::pair<CAddressIndexKey, CAmount> > &addressIndex,
                     int start = 0, int end = 0);
bool GetAddressUnspent(uint256 addressHash, int type,
                       std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > &unspentOutputs);


#endif // BITCOIN_INSIGHT_INSIGHT_H
