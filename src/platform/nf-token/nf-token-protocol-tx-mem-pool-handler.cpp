// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "platform/specialtx.h"
#include "nf-token-protocol-tx-mem-pool-handler.h"

namespace Platform
{
    bool NftProtoTxMemPoolHandler::RegisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator)
    {
        return handlerRegistrator.RegisterHandler(TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER, this);
    }

    void NftProtoTxMemPoolHandler::UnregisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator)
    {
        handlerRegistrator.UnregisterHandler(TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER);
    }

    bool NftProtoTxMemPoolHandler::ExistsTxConflict(const CTransaction & tx) const
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER);

        auto nftProto = GetNftProtoRegTx(tx).GetNftProto();
        return m_nftProtoPoolTxMap.find(nftProto.tokenProtocolId) != m_nftProtoPoolTxMap.end();
    }

    bool NftProtoTxMemPoolHandler::AddMemPoolTx(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER);

        auto nftProto = GetNftProtoRegTx(tx).GetNftProto();
        return m_nftProtoPoolTxMap.emplace(nftProto.tokenProtocolId, tx.GetHash()).second;
    }

    void NftProtoTxMemPoolHandler::RemoveMemPoolTx(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER);

        auto nftProto = GetNftProtoRegTx(tx).GetNftProto();
        m_nftProtoPoolTxMap.erase(nftProto.tokenProtocolId);
    }

    uint256 NftProtoTxMemPoolHandler::GetConflictTxIfExists(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_PROTOCOL_REGISTER);

        auto nftProto = GetNftProtoRegTx(tx).GetNftProto();
        auto memPoolInfoIt = m_nftProtoPoolTxMap.find(nftProto.tokenProtocolId);

        if (memPoolInfoIt != m_nftProtoPoolTxMap.end())
        {
            if (memPoolInfoIt->second != tx.GetHash())
                return memPoolInfoIt->second;
        }
        return uint256();
    }

    NfTokenProtocolRegTx NftProtoTxMemPoolHandler::GetNftProtoRegTx(const CTransaction & tx)
    {
        NfTokenProtocolRegTx nftProtoRegTx;
        bool result = GetTxPayload(tx, nftProtoRegTx);
        // should have been checked already
        assert(result);
        return nftProtoRegTx;
    }
}