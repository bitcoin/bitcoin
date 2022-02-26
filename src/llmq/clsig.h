// Copyright (c) 2019-2021 The Dash Core developers
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
    public:
        int32_t nHeight{-1};
        uint256 blockHash;
        CBLSSignature sig;

    public:
        SERIALIZE_METHODS(CChainLockSig, obj)
        {
            READWRITE(obj.nHeight, obj.blockHash, obj.sig);
        }

        bool IsNull() const;
        std::string ToString() const;
    };
} // namespace llmq

#endif // BITCOIN_LLMQ_CLSIG_H
