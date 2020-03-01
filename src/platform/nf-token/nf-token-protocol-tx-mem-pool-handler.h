// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKEN_NF_TOKEN_PROTOCOL_TX_MEM_POOL_HANDLER_H
#define CROWN_PLATFORM_NF_TOKEN_NF_TOKEN_PROTOCOL_TX_MEM_POOL_HANDLER_H

#include <unordered_map>
#include "platform/specialtx-common.h"
#include "nf-token-protocol-reg-tx.h"

namespace Platform
{
    class NftProtoTxMemPoolHandler : public SpecTxMemPoolHandler
    {
    public:
        bool RegisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) override;
        void UnregisterSelf(SpecTxMemPoolHandlerRegistrator & handlerRegistrator) override;
        bool ExistsTxConflict(const CTransaction & tx) const override;
        bool AddMemPoolTx(const CTransaction & tx) override;
        void RemoveMemPoolTx(const CTransaction & tx) override;
        uint256 GetConflictTxIfExists(const CTransaction & tx) override;

    private:
        static NfTokenProtocolRegTx GetNftProtoRegTx(const CTransaction & tx);

    private:
        std::unordered_map<uint64_t, uint256> m_nftProtoPoolTxMap;
    };
}

#endif //CROWN_PLATFORM_NF_TOKEN_NF_TOKEN_PROTOCOL_TX_MEM_POOL_HANDLER_H
