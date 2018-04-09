// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_SIMPLIFIEDMNS_H
#define DASH_SIMPLIFIEDMNS_H

#include "serialize.h"
#include "pubkey.h"
#include "netaddress.h"

class UniValue;
class CDeterministicMNList;
class CDeterministicMN;

class CSimplifiedMNListEntry
{
public:
    uint256 proRegTxHash;
    CService service;
    CKeyID keyIDOperator;
    CKeyID keyIDVoting;
    bool isValid;

public:
    CSimplifiedMNListEntry() {}
    CSimplifiedMNListEntry(const CDeterministicMN& dmn);

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(proRegTxHash);
        READWRITE(service);
        READWRITE(keyIDOperator);
        READWRITE(keyIDVoting);
        READWRITE(isValid);
    }

public:
    uint256 CalcHash() const;

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};

class CSimplifiedMNList
{
public:
    std::vector<CSimplifiedMNListEntry> mnList;

public:
    CSimplifiedMNList() {}
    CSimplifiedMNList(const CDeterministicMNList& dmnList);

    uint256 CalcMerkleRoot(bool *pmutated = NULL) const;
};

#endif//DASH_SIMPLIFIEDMNS_H
