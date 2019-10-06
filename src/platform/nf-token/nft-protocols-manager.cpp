// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nft-protocols-manager.h"

namespace Platform
{
    /*static*/ std::unique_ptr<NftProtocolsManager> NftProtocolsManager::s_instance;

    bool NftProtocolsManager::AddNftProto(const NfTokenProtocol & nftProto, const CTransaction & tx, const CBlockIndex * pindex)
    {
        LOCK(m_cs);
        assert(nftProto.tokenProtocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!nftProto.tokenProtocolOwnerId.IsNull());
        assert(pindex != nullptr);
        assert(!tx.GetHash().IsNull());

        std::shared_ptr<NfTokenProtocol> nftProtoPtr(new NfTokenProtocol(nftProto));
        NftProtoIndex nftProtoIndex(pindex, tx.GetHash(), nftProtoPtr);
        auto itRes = m_nftProtoIndexSet.emplace(std::move(nftProtoIndex));

        if (itRes.second)
        {
            NftProtoDiskIndex nftProtoDiskIndex(*pindex->phashBlock, pindex, tx.GetHash(), nftProtoPtr);
            //TODO: db PlatformDb::Instance().WriteNftDiskIndex(nftDiskIndex);
            //this->UpdateTotalSupply(nfTokenPtr->tokenProtocolId, true);
        }
        return itRes.second;
    }

    bool NftProtocolsManager::Contains(uint64_t protocolId)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        return this->Contains(protocolId, m_tipHeight);
    }

    bool NftProtocolsManager::Contains(uint64_t protocolId, int height)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(height >= 0);

        auto nftProtoIdx = this->GetNfTokenProtoIndex(protocolId);
        if (!nftProtoIdx.IsNull())
            return nftProtoIdx.BlockIndex()->nHeight <= height;
        return false;
    }

    NftProtoIndex NftProtocolsManager::GetNfTokenProtoIndex(uint64_t protocolId)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);

        auto it = m_nftProtoIndexSet.find(protocolId);
        if (it != m_nftProtoIndexSet.end())
        {
            return *it;
        }
        return NftProtoIndex();
        /// PlatformDb::Instance().OptimizeRam() is on
        /// TODO: DB return GetNftIndexFromDb(protocolId, tokenId);
    }

    CKeyID NftProtocolsManager::OwnerOf(uint64_t protocolId)
    {
        return CKeyID(); //TODO
    }

    void NftProtocolsManager::ProcessFullNftProtoIndexRange(std::function<bool(const NftProtoIndex &)> nftIndexHandler) const
    {

    }

    void NftProtocolsManager::ProcessNftProtoIndexRangeByHeight(std::function<bool(const NftProtoIndex &)> nftIndexHandler,
                                                                                   int height,
                                                                                   int count,
                                                                                   int startFrom) const
    {

    }

    bool NftProtocolsManager::Delete(uint64_t protocolId)
    {
        return false;
    }

    bool NftProtocolsManager::Delete(uint64_t protocolId, int height)
    {
        return false;
    }

    void NftProtocolsManager::UpdateBlockTip(const CBlockIndex * pindex)
    {

    }
}