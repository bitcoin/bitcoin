#include "platform/specialtx.h"
#include "nf-token-tx-mem-pool-handler.h"

namespace Platform
{
    bool NfTokenTxMemPoolHandler::RegisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator)
    {
        return handlerRegistrator.RegisterHandler(TRANSACTION_NF_TOKEN_REGISTER, this);
    }

    void NfTokenTxMemPoolHandler::UnregisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator)
    {
        handlerRegistrator.UnregisterHandler(TRANSACTION_NF_TOKEN_REGISTER);
    }

    bool NfTokenTxMemPoolHandler::ExistsTxConflict(const CTransaction & tx) const
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_REGISTER);

        auto nfToken = GetNfTokenRegTx(tx).GetNfToken();
        return m_nfTokenTxSet.find(std::make_tuple(nfToken.tokenProtocolId, nfToken.tokenId)) != m_nfTokenTxSet.end();
    }

    bool NfTokenTxMemPoolHandler::AddMemPoolTx(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_REGISTER);

        auto nfToken = GetNfTokenRegTx(tx).GetNfToken();
        NfTokenTxMemPoolInfo memPoolInfo{nfToken.tokenProtocolId, nfToken.tokenId, tx.GetHash()};

        return m_nfTokenTxSet.emplace(std::move(memPoolInfo)).second;
    }

    void NfTokenTxMemPoolHandler::RemoveMemPoolTx(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_REGISTER);

        auto nfToken = GetNfTokenRegTx(tx).GetNfToken();
        auto memPoolInfoIt = m_nfTokenTxSet.find(std::make_tuple(nfToken.tokenProtocolId, nfToken.tokenId));

        m_nfTokenTxSet.erase(memPoolInfoIt);
    }

    uint256 NfTokenTxMemPoolHandler::GetConflictTxIfExists(const CTransaction & tx)
    {
        assert(tx.nType == TRANSACTION_NF_TOKEN_REGISTER);

        auto nfToken = GetNfTokenRegTx(tx).GetNfToken();
        auto memPoolInfoIt = m_nfTokenTxSet.find(std::make_tuple(nfToken.tokenProtocolId, nfToken.tokenId));

        if (memPoolInfoIt != m_nfTokenTxSet.end())
        {
            if (memPoolInfoIt->regTxHash != tx.GetHash())
                return memPoolInfoIt->regTxHash;
        }
        return uint256();
    }

    NfTokenRegTx NfTokenTxMemPoolHandler::GetNfTokenRegTx(const CTransaction & tx)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);
        return nfTokenRegTx;
    }
}
