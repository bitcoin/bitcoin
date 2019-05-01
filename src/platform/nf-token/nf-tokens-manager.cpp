// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "chain.h"

#include "platform/specialtx.h"
#include "platform/platform-db.h"
#include "nf-tokens-manager.h"

namespace Platform
{
    using NftIndexByProtocolAndOwnerId = NfTokensIndexSet::index<Tags::ProtocolIdOwnerId>::type;
    using NftIndexByProtocolId = NfTokensIndexSet::index<Tags::ProtocolId>::type;
    using NftIndexByOwnerId = NfTokensIndexSet::index<Tags::OwnerId>::type;

    /*static*/ std::unique_ptr<NfTokensManager> NfTokensManager::s_instance;

    NfTokensManager::NfTokensManager()
    {
        if (PlatformDb::Instance().OptimizeSpeed())
        {
            PlatformDb::Instance().ProcessNftIndexGuts([this](NfTokenIndex nftIndex) -> bool
            {
                return m_nfTokensIndexSet.emplace(std::move(nftIndex)).second;
            });
        }
    }

    bool NfTokensManager::AddNfToken(const NfToken & nfToken, const CTransaction & tx, const CBlockIndex * pindex)
    {
        assert(nfToken.tokenProtocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!nfToken.tokenId.IsNull());
        assert(!nfToken.tokenOwnerKeyId.IsNull());
        assert(!nfToken.metadataAdminKeyId.IsNull());
        assert(pindex != nullptr);
        assert(!tx.GetHash().IsNull());

        std::shared_ptr<NfToken> nfTokenPtr(new NfToken(nfToken));
        NfTokenIndex nftIndex(pindex, tx.GetHash(), nfTokenPtr);
        auto itRes = m_nfTokensIndexSet.emplace(std::move(nftIndex));

        if (itRes.second)
        {
            NfTokenDiskIndex nftDiskIndex(*pindex->phashBlock, pindex, tx.GetHash(), nfTokenPtr);
            PlatformDb::Instance().WriteNftDiskIndex(nftDiskIndex);
        }
        return itRes.second;
    }

    NfTokenIndex NfTokensManager::GetNfTokenIndex(const uint64_t & protocolId, const uint256 & tokenId)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));
        if (it != m_nfTokensIndexSet.end())
        {
            return *it;
        }
        /// Should be called only when RAM optimization is on
        return GetNftIndexFromDb(protocolId, tokenId);
    }

    std::weak_ptr<const NfToken> NfTokensManager::GetNfToken(const uint64_t & protocolId, const uint256 & tokenId)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));
        if (it != m_nfTokensIndexSet.end())
        {
            return it->NfTokenPtr();
        }
        /// Should be called only when RAM optimization is on
        return GetNftIndexFromDb(protocolId, tokenId).NfTokenPtr();
    }

    bool NfTokensManager::Contains(const uint64_t &protocolId, const uint256 &tokenId, int height)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());
        assert(height >= 0);

        auto nfTokenIdx = this->GetNfTokenIndex(protocolId, tokenId);
        if (!nfTokenIdx.IsNull())
            return nfTokenIdx.BlockIndex()->nHeight <= height;
        return false;
    }

    bool NfTokensManager::Contains(const uint64_t & protocolId, const uint256 & tokenId)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());
        return this->Contains(protocolId, tokenId, m_tipHeight);
    }

    CKeyID NfTokensManager::OwnerOf(const uint64_t & protocolId, const uint256 & tokenId)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));
        if (it != m_nfTokensIndexSet.end())
        {
            return it->NfTokenPtr()->tokenOwnerKeyId;
        }
        /// Should be called only when RAM optimization is on
        return GetNftIndexFromDb(protocolId, tokenId).NfTokenPtr()->tokenOwnerKeyId;
    }

    std::size_t NfTokensManager::BalanceOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        if (PlatformDb::Instance().OptimizeSpeed())
        {
            const NftIndexByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensIndexSet.get<Tags::ProtocolIdOwnerId>();
            return protocolOwnerIndex.count(std::make_tuple(protocolId, ownerId));
        }
        else /// (PlatformDb::Instance().OptimizeRam())
        {
            std::size_t count = 0;
            PlatformDb::Instance().ProcessNftIndexGuts([&](NfTokenIndex nftIndex) -> bool
            {
                if (nftIndex.NfTokenPtr()->tokenProtocolId == protocolId &&
                    nftIndex.NfTokenPtr()->tokenOwnerKeyId == ownerId)
                {
                    count++;
                }
            });
            return count;
        }
    }

    std::size_t NfTokensManager::BalanceOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        if (PlatformDb::Instance().OptimizeSpeed())
        {
            const NftIndexByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
            return ownerIndex.count(ownerId);
        }
        else /// (PlatformDb::Instance().OptimizeRam())
        {
            std::size_t count = 0;
            PlatformDb::Instance().ProcessNftIndexGuts([&](NfTokenIndex nftIndex) -> bool
            {
                if (nftIndex.NfTokenPtr()->tokenOwnerKeyId == ownerId)
                    count++;
            });
            return count;
        }
    }

    std::vector<std::weak_ptr<const NfToken> > NfTokensManager::NfTokensOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        const NftIndexByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensIndexSet.get<Tags::ProtocolIdOwnerId>();
        const auto range = protocolOwnerIndex.equal_range(std::make_tuple(protocolId, ownerId));

        std::vector<std::weak_ptr<const NfToken> > nfTokens;
        nfTokens.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [&](const NfTokenIndex & nfTokenIdx)
        {
            nfTokens.emplace_back(nfTokenIdx.NfTokenPtr());
        });

        return nfTokens;
    }

    std::vector<std::weak_ptr<const NfToken> > NfTokensManager::NfTokensOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NftIndexByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
        const auto range = ownerIndex.equal_range(ownerId);

        std::vector<std::weak_ptr<const NfToken> > nfTokens;
        nfTokens.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [&](const NfTokenIndex & nfTokenIdx)
        {
            nfTokens.emplace_back(nfTokenIdx.NfTokenPtr());
        });

        return nfTokens;
    }

    std::vector<uint256> NfTokensManager::NfTokenIdsOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        const NftIndexByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensIndexSet.get<Tags::ProtocolIdOwnerId>();
        const auto range = protocolOwnerIndex.equal_range(std::make_tuple(protocolId, ownerId));

        std::vector<uint256> nfTokenIds;
        nfTokenIds.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [&](const NfTokenIndex & nfTokenIdx)
        {
            nfTokenIds.emplace_back(nfTokenIdx.NfTokenPtr()->tokenId);
        });
        return nfTokenIds;
    }

    std::vector<uint256> NfTokensManager::NfTokenIdsOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NftIndexByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
        const auto range = ownerIndex.equal_range(ownerId);

        std::vector<uint256> nfTokenIds;
        nfTokenIds.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [&](const NfTokenIndex & nfTokenIdx)
        {
            nfTokenIds.emplace_back(nfTokenIdx.NfTokenPtr()->tokenId);
        });
        return nfTokenIds;
    }

    std::size_t NfTokensManager::TotalSupply() const
    {
        return m_nfTokensIndexSet.size();
    }

    std::size_t NfTokensManager::TotalSupply(const uint64_t & protocolId) const
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);

        const NftIndexByProtocolId & protocolIndex = m_nfTokensIndexSet.get<Tags::ProtocolId>();
        return protocolIndex.count(protocolId);
    }

    NfTokensManager::NftIndexRange NfTokensManager::FullNftIndexRange() const
    {
        //TODO: read from the db if optimized by RAM
        return m_nfTokensIndexSet;
    }

    NfTokensManager::NftIndexRange NfTokensManager::NftIndexRangeByHeight(int height) const
    {
        //TODO: read from the db if optimized by RAM
        return m_nfTokensIndexSet.get<Tags::Height>().range(
                    bmx::unbounded,
                    [&](int curHeight) { return curHeight <= height; }
        );
    }

    bool NfTokensManager::Delete(const uint64_t & protocolId, const uint256 & tokenId)
    {
        return Delete(protocolId, tokenId, m_tipHeight);
    }

    bool NfTokensManager::Delete(const uint64_t & protocolId, const uint256 & tokenId, int height)
    {
        assert(protocolId != NfToken::UNKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());
        assert(height >= 0);

        auto it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));
        if (it != m_nfTokensIndexSet.end() && it->BlockIndex()->nHeight <= height)
        {
            m_nfTokensIndexSet.erase(it);
            PlatformDb::Instance().EraseNftDiskIndex(protocolId, tokenId);
            return true;
        }
        return false;
    }

    void NfTokensManager::UpdateBlockTip(const CBlockIndex * pindex)
    {
        assert(pindex != nullptr);
        if (pindex != nullptr)
        {
            m_tipHeight = pindex->nHeight;
            m_tipBlockHash = pindex->GetBlockHash();
        }
    }

    NfTokenIndex NfTokensManager::GetNftIndexFromDb(const uint64_t & protocolId, const uint256 & tokenId)
    {
        NfTokenIndex nftIndex = PlatformDb::Instance().ReadNftIndex(protocolId, tokenId);
        if (!nftIndex.IsNull())
        {
            auto insRes = m_nfTokensIndexSet.emplace(std::move(nftIndex));
            assert(insRes.second);
            return *insRes.first;
        }
        return nftIndex;
    }
}
