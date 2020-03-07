// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_INDEX_H
#define CROWN_PLATFORM_NF_TOKEN_INDEX_H

#include <memory>
#include "nf-token.h"

class CBlockIndex;

namespace Platform
{
    class NfTokenIndex
    {
    protected:
        const CBlockIndex * m_blockIndex;
        uint256 m_regTxHash;
        std::shared_ptr<NfToken> m_nfToken;

    public:
        NfTokenIndex(const CBlockIndex * blockIndex,
                     const uint256 & regTxHash,
                     const std::shared_ptr<NfToken> & nfToken)
                : m_blockIndex(blockIndex)
                , m_regTxHash(regTxHash)
                , m_nfToken(nfToken)
        {
        }

        NfTokenIndex()
        {
            SetNull();
        }

        const CBlockIndex * BlockIndex() const { return m_blockIndex; }
        const uint256 & RegTxHash() const { return m_regTxHash; }
        const std::shared_ptr<NfToken> & NfTokenPtr() const { return m_nfToken; }

        void SetNull()
        {
            m_blockIndex = nullptr;
            m_regTxHash.SetNull();
            m_nfToken.reset(new NfToken());
        }

        bool IsNull()
        {
            return m_blockIndex == nullptr && m_regTxHash.IsNull() && m_nfToken->tokenId.IsNull();
        }
    };

    class NfTokenDiskIndex : public NfTokenIndex
    {
    private:
        uint256 m_blockHash;

    public:
        NfTokenDiskIndex(const uint256 & blockHash,
                         const CBlockIndex * blockIndex,
                         const uint256 & regTxHash,
                         const std::shared_ptr<NfToken> & nfToken)
                : NfTokenIndex(blockIndex, regTxHash, nfToken)
                , m_blockHash(blockHash)
        {
        }

        NfTokenDiskIndex()
        {
            m_blockHash.SetNull();
            SetNull();
        }

        const uint256 & BlockHash() const { return m_blockHash; }

    public:
        ADD_SERIALIZE_METHODS
        template<typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
        {
            READWRITE(m_blockHash);
            READWRITE(m_regTxHash);
            READWRITE(*m_nfToken);
        }
    };
}

#endif //CROWN_PLATFORM_NF_TOKEN_INDEX_H
