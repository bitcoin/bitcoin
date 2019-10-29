// Copyright (c) 2017-2018 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_SPECIAL_CBTX_H
#define BITGREEN_SPECIAL_CBTX_H

#include <consensus/validation.h>
#include <primitives/transaction.h>

class CBlock;
class CBlockIndex;
class UniValue;

// coinbase transaction
class CCbTx
{
public:
    static const uint16_t CURRENT_VERSION = 2;

public:
    uint16_t nVersion{CURRENT_VERSION};
    int32_t nHeight{0};
    uint256 merkleRootMNList;
    uint256 merkleRootQuorums;

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nVersion);
        READWRITE(nHeight);
        READWRITE(merkleRootMNList);

        if (nVersion >= 2) {
            READWRITE(merkleRootQuorums);
        }
    }

    std::string ToString() const;
    void ToJson(UniValue& obj) const;
};

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state);
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, CValidationState& state);
bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state);
bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state);

#endif //BITGREEN_SPECIAL_CBTX_H
