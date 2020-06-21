// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_EVO_CBTX_H
#define SYSCOIN_EVO_CBTX_H

#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <univalue.h>

class CBlock;
class CBlockIndex;

// coinbase transaction
class CCbTx
{
public:
    static const uint16_t CURRENT_VERSION = 2;

public:
    uint16_t nVersion{CURRENT_VERSION};
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;

public:
    SERIALIZE_METHODS(CCbTx, obj) {
        READWRITE(obj.nVersion, obj.merkleRootMNList, obj.merkleRootQuorums);
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("merkleRootMNList", merkleRootMNList.ToString());
        obj.pushKV("merkleRootQuorums", merkleRootQuorums.ToString());
    }
};

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state);

bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, CValidationState& state);
bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state);
bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state);

#endif //SYSCOIN_EVO_CBTX_H
