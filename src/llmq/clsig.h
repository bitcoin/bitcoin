// Copyright (c) 2019-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_CLSIG_H
#define BITCOIN_LLMQ_CLSIG_H

#include <bls/bls.h>
#include <serialize.h>
#include <uint256.h>

namespace llmq
{

extern const std::string CLSIG_REQUESTID_PREFIX;

class CChainLockSig
{
private:
    int32_t nHeight{-1};
    uint256 blockHash;
    CBLSSignature sig;

public:
    CChainLockSig(int32_t nHeight, const uint256& blockHash, const CBLSSignature& sig) :
        nHeight(nHeight),
        blockHash(blockHash),
        sig(sig)
    {}
    CChainLockSig() = default;


    [[nodiscard]] int32_t getHeight() const;
    [[nodiscard]] const uint256& getBlockHash() const;
    [[nodiscard]] const CBLSSignature& getSig() const;
    [[nodiscard]] bool IsNull() const;
    [[nodiscard]] std::string ToString() const;

    SERIALIZE_METHODS(CChainLockSig, obj)
    {
        READWRITE(obj.nHeight, obj.blockHash, obj.sig);
    }
};
} // namespace llmq

#endif // BITCOIN_LLMQ_CLSIG_H
