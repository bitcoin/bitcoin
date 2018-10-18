// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "chain.h"
#include "nf-tokens-manager.h"

namespace Platform
{
    using NfTokensIndexSetByProtocolAndOwnerId = NfTokensIndexSet::index<Tags::ProtocolIdOwnerId>::type;
    using NfTokensIndexSetByProtocolId = NfTokensIndexSet::index<Tags::ProtocolId>::type;
    using NfTokensIndexSetByOwnerId = NfTokensIndexSet::index<Tags::OwnerId>::type;

    /*static*/ std::unique_ptr<NfTokensEntireSetManager> NfTokensEntireSetManager::s_instance;

    bool NfTokensEntireSetManager::AddNfToken(const NfToken & nfToken, const CTransaction & tx, const CBlockIndex * pindex)
    {
        assert(nfToken.tokenProtocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!nfToken.tokenId.IsNull());
        assert(!nfToken.tokenOwnerKeyId.IsNull());
        assert(!nfToken.metadataAdminKeyId.IsNull());

        NfTokenIndex newNfTokenIndex;
        newNfTokenIndex.blockHash = *pindex->phashBlock;
        newNfTokenIndex.height = pindex->nHeight;
        newNfTokenIndex.regTxHash = tx.GetHash();
        newNfTokenIndex.nfToken.reset(new NfToken(nfToken));

        //NfTokenIndex newNfTokenIndex {*pindex->phashBlock, tx.GetHash(), std::shared_ptr<NfToken>(new NfToken(nfToken))};

        return m_nfTokensIndexSet.emplace(std::move(newNfTokenIndex)).second;

        //return m_nfTokensSet.emplace( {*pindex->phashBlock, tx.GetHash(), std::shared_ptr<NfToken>(new NfToken(nfToken))} );
    }

    const NfTokenIndex * NfTokensEntireSetManager::GetNfTokenIndex(const uint64_t & protocolId, const uint256 & tokenId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));

        if (it != m_nfTokensIndexSet.end())
        {
            return &(*it);
        }
        return nullptr;
    }

    std::weak_ptr<const NfToken> NfTokensEntireSetManager::GetNfToken(const uint64_t & protocolId, const uint256 & tokenId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));

        if (it != m_nfTokensIndexSet.end())
        {
            return it->nfToken;
        }
        return nullptr;
    }

    bool NfTokensEntireSetManager::ContainsAtHeight(const uint64_t &protocolId, const uint256 &tokenId, int height) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());
        assert(height >= 0);

        auto nfTokenIdx = this->GetNfTokenIndex(protocolId, tokenId);
        if (nfTokenIdx != nullptr)
            return nfTokenIdx.height <= height;
        return false;
    }

    bool NfTokensEntireSetManager::Contains(const uint64_t & protocolId, const uint256 & tokenId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        return m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId)) != m_nfTokensIndexSet.end();
    }

    CKeyID NfTokensEntireSetManager::OwnerOf(const uint64_t & protocolId, const uint256 & tokenId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!tokenId.IsNull());

        NfTokensIndexSet::const_iterator it = m_nfTokensIndexSet.find(std::make_tuple(protocolId, tokenId));
        return it->nfToken->tokenOwnerKeyId;
    }

    std::size_t NfTokensEntireSetManager::BalanceOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensIndexSet.get<Tags::ProtocolIdOwnerId>();
        return protocolOwnerIndex.count(std::make_tuple(protocolId, ownerId));
    }

    std::vector<std::weak_ptr<const NfToken> > NfTokensEntireSetManager::NfTokensOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensSet.get<Tags::ProtocolIdOwnerId>();
        const auto range = protocolOwnerIndex.equal_range(std::make_tuple(protocolId, ownerId));

        std::vector<std::weak_ptr<const NfToken> > nfTokens;
        //nfTokens.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [](const NfTokenIndex & nfTokenIdx)
        {
            nfTokens.emplace_back(nfTokenIdx.nfToken);
        });
    }

    std::vector<uint256> NfTokensEntireSetManager::NfTokenIdsOf(const uint64_t & protocolId, const CKeyID & ownerId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByProtocolAndOwnerId & protocolOwnerIndex = m_nfTokensSet.get<Tags::ProtocolIdOwnerId>();
        const auto range = protocolOwnerIndex.equal_range(std::make_tuple(protocolId, ownerId));

        std::vector<uint256> nfTokenIds;
        //nfTokenIds.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [](const NfTokenIndex & nfTokenIdx)
        {
            nfTokenIds.emplace_back(nfTokenIdx.nfToken->tokenId);
        });
        return nfTokenIds;
    }

    std::size_t NfTokensEntireSetManager::BalanceOf(const CKeyID & ownerId)
    {
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
        return ownerIndex.count(ownerId);
    }

    std::vector<std::weak_ptr<const NfToken> > NfTokensEntireSetManager::NfTokensOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
        const auto range = ownerIndex.equal_range(ownerId);

        std::vector<std::weak_ptr<const NfToken> > nfTokens;
        //nfTokens.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [](const NfTokenIndex & nfTokenIdx)
        {
            nfTokens.emplace_back(nfTokenIdx.nfToken);
        });
    }

    std::vector<uint256> NfTokensEntireSetManager::NfTokenIdsOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NfTokensIndexSetByOwnerId & ownerIndex = m_nfTokensIndexSet.get<Tags::OwnerId>();
        const auto range = ownerIndex.equal_range(ownerId);

        std::vector<uint256> nfTokenIds;
        //nfTokenIds.reserve(std::distance(range.first, range.second));
        std::for_each(range.first, range.second, [](const NfTokenIndex & nfTokenIdx)
        {
            nfTokenIds.emplace_back(nfTokenIdx.nfToken->tokenId);
        });
        return nfTokenIds;
    }

    std::size_t NfTokensEntireSetManager::TotalSupply() const
    {
        return m_nfTokensIndexSet.size_();
    }

    std::size_t NfTokensEntireSetManager::TotalSupply(const uint64_t & protocolId) const
    {
        assert(protocolId != NfToken::UKNOWN_TOKEN_PROTOCOL);

        const NfTokensIndexSetByProtocolId & protocolIndex = m_nfTokensIndexSet.get<Tags::ProtocolId>();
        return protocolIndex.count(protocolId);
    }
}
