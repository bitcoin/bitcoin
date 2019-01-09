#ifndef CROWN_PLATFORM_NF_TOKEN_TX_MEM_POOL_HANDLER_H
#define CROWN_PLATFORM_NF_TOKEN_TX_MEM_POOL_HANDLER_H

#include "platform/specialtx-common.h"
#include "nf-token-multiindex-utils.h"
#include "nf-token-reg-tx.h"

namespace Platform
{
    class NfTokenTxMemPoolInfo
    {
    public:
        uint64_t tokenProtocolId;
        uint256 tokenId;
        uint256 regTxHash;
    };

    struct TokenProtocolIdMemPoolExtractor
    {
        using result_type = uint64_t;

        result_type operator()(const NfTokenTxMemPoolInfo & nfTokenTxInfo) const
        {
            return nfTokenTxInfo.tokenProtocolId;
        }
    };

    struct TokenIdMemPoolExtractor
    {
        using result_type = uint256;

        result_type operator()(const NfTokenTxMemPoolInfo & nfTokenTxInfo) const
        {
            return nfTokenTxInfo.tokenId;
        }
    };

    using NfTokenTxMemPoolSet = bmx::multi_index_container<
        NfTokenTxMemPoolInfo,
        bmx::indexed_by<
            bmx::hashed_unique<
                bmx::tag<Tags::ProtocolIdTokenId>,
                bmx::composite_key<
                    NfTokenTxMemPoolInfo,
                    TokenProtocolIdMemPoolExtractor,
                    TokenIdMemPoolExtractor
                >
            >
        >
    >;

    class NfTokenTxMemPoolHandler : public SpecTxMemPoolHandler
    {
    public:
        bool RegisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) override;
        void UnregisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) override;
        bool ExistsTxConflict(const CTransaction & tx) const override;
        bool AddMemPoolTx(const CTransaction & tx) override;
        void RemoveMemPoolTx(const CTransaction & tx) override;
        uint256 GetConflictTxIfExists(const CTransaction & tx) override;

    private:
        static NfTokenRegTx GetNfTokenRegTx(const CTransaction & tx);

    private:
        NfTokenTxMemPoolSet m_nfTokenTxSet;
    };
}

#endif // CROWN_PLATFORM_NF_TOKEN_TX_MEM_POOL_HANDLER_H
