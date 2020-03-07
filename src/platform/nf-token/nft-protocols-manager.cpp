// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include "nft-protocols-manager.h"
#include "platform/platform-db.h"

namespace Platform
{
    /*static*/ std::unique_ptr<NftProtocolsManager> NftProtocolsManager::s_instance;

    NftProtocolsManager::NftProtocolsManager()
    {
        if (chainActive.Tip() != nullptr)
        {
            m_tipHeight = chainActive.Tip()->nHeight;
            m_tipBlockHash = chainActive.Tip()->GetBlockHash();
        }

        PlatformDb::Instance().ReadTotalProtocolCount(m_totalProtocolsCount);

        PlatformDb::Instance().ProcessNftProtoIndexGutsOnly([this](NftProtoIndex protoIndex) -> bool
        {
            return m_nftProtoIndexSet.emplace(std::move(protoIndex)).second;
        });
    }

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
            NftProtoDiskIndex protoDiskIndex(*pindex->phashBlock, pindex, tx.GetHash(), nftProtoPtr);
            PlatformDb::Instance().WriteNftProtoDiskIndex(protoDiskIndex);
            PlatformDb::Instance().WriteTotalProtocolCount(++m_totalProtocolsCount);
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

        auto nftProtoIdx = this->GetNftProtoIndex(protocolId);
        if (!nftProtoIdx.IsNull())
            return nftProtoIdx.BlockIndex()->nHeight <= height;
        return false;
    }

    NftProtoIndex NftProtocolsManager::GetNftProtoIndex(uint64_t protocolId)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);

        auto it = m_nftProtoIndexSet.find(protocolId);
        if (it != m_nftProtoIndexSet.end())
        {
            return *it;
        }

        return NftProtoIndex();
    }

    NftProtoIndex NftProtocolsManager::GetNftProtoIndex(const uint256 & regTxId)
    {
        LOCK(m_cs);
        assert(!regTxId.IsNull());

        const auto it = m_nftProtoIndexSet.get<Tags::RegTxHash>().find(regTxId);
        if (it != m_nftProtoIndexSet.get<Tags::RegTxHash>().end())
        {
            return *it;
        }
        return NftProtoIndex();
    }

    CKeyID NftProtocolsManager::OwnerOf(uint64_t protocolId)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);

        auto it = m_nftProtoIndexSet.find(protocolId);
        if (it != m_nftProtoIndexSet.end())
        {
            return it->NftProtoPtr()->tokenProtocolOwnerId;;
        }
        return CKeyID();
    }

    std::size_t NftProtocolsManager::TotalSupply() const
    {
        LOCK(m_cs);
        return m_totalProtocolsCount;
    }

    void NftProtocolsManager::ProcessFullNftProtoIndexRange(std::function<bool(const NftProtoIndex &)> protoIndexHandler) const
    {
        LOCK(m_cs);
        for (const auto & protoIndex : m_nftProtoIndexSet)
        {
            if (!protoIndexHandler(protoIndex))
                LogPrintf("%s: NFT proto index processing failed.", __func__);
        }
    }

    void NftProtocolsManager::ProcessNftProtoIndexRangeByHeight(std::function<bool(const NftProtoIndex &)> protoIndexHandler,
                                                                                   unsigned int height,
                                                                                   unsigned int count,
                                                                                   unsigned int skipFromTip) const
    {
        LOCK(m_cs);
        auto originalRange = m_nftProtoIndexSet.get<Tags::Height>().range(
                bmx::unbounded,
                [&](unsigned int curHeight) { return curHeight <= height; }
        );

        unsigned long rangeSize = std::distance(originalRange.first, originalRange.second);
        assert(rangeSize >= 0);

        auto begin = skipFromTip + count > rangeSize ? originalRange.first : std::prev(originalRange.second, skipFromTip + count);
        auto end = skipFromTip > rangeSize ? originalRange.first : std::prev(originalRange.second, skipFromTip);

        NftProtoIndexRange finalRange(begin, end);
        for (const auto & protoIndex : finalRange)
        {
            if (!protoIndexHandler(protoIndex))
                LogPrintf("%s: NFT proto index processing failed.", __func__);
        }
    }

    bool NftProtocolsManager::Delete(uint64_t protocolId)
    {
        return Delete(protocolId, m_tipHeight);
    }

    bool NftProtocolsManager::Delete(uint64_t protocolId, int height)
    {
        LOCK(m_cs);
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(height >= 0);

        auto it = m_nftProtoIndexSet.find(protocolId);
        if (it != m_nftProtoIndexSet.end() && it->BlockIndex()->nHeight <= height)
        {
            m_nftProtoIndexSet.erase(it);
            PlatformDb::Instance().EraseNftProtoDiskIndex(protocolId);
            PlatformDb::Instance().WriteTotalProtocolCount(--m_totalProtocolsCount);
            return true;
        }

        return false;
    }

    void NftProtocolsManager::UpdateBlockTip(const CBlockIndex * pindex)
    {
        LOCK(m_cs);
        assert(pindex != nullptr);
        if (pindex != nullptr)
        {
            m_tipHeight = pindex->nHeight;
            m_tipBlockHash = pindex->GetBlockHash();
        }
    }

    /// Reserved for future use
    /* NftProtoIndex NftProtocolsManager::GetNftProtoIndexFromDb(uint64_t protocolId)
    {
        NftProtoIndex protoIndex = PlatformDb::Instance().ReadNftProtoIndex(protocolId);
        if (!protoIndex.IsNull())
        {
            auto insRes =  m_nftProtoIndexSet.emplace(std::move(protoIndex));
            assert(insRes.second);
            return *insRes.first;
        }
        else
        {
            LogPrintf("%s: Can't read NFT proto index %s from the database", __func__, std::to_string(protocolId));
            return protoIndex;
        }
    }*/
}