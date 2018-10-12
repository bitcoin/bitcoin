// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "nf-tokens-single-protocol-manager.h"

namespace Platform
{
    using NfTokenSetByOwnerId = NfTokenSet::index<Tags::OwnerId>::type;

    NfTokensSingleProtocolManager::NfTokensSingleProtocolManager(const uint64_t & protocolId)
        : m_protocolId(protocolId)
    {
    }

    bool NfTokensSingleProtocolManager::Contains(const uint256 & tokenId) const
    {
        assert(!tokenId.IsNull());

        return (m_nfTokensSet.find(tokenId) != m_nfTokensSet.end());
    }

    //TODO: should use std::move?
    bool NfTokensSingleProtocolManager::AddNfToken(NfToken nfToken)
    {
        assert(nfToken.tokenProtocolId == m_protocolId);

        if (nfToken.tokenProtocolId != m_protocolId)
        {
            return false;
        }
        return (m_nfTokensSet.insert(nfToken)).second;
    }

    uint64_t NfTokensSingleProtocolManager::GetProtocolId() const
    {
        return m_protocolId;
    }

    // TODO: use nullptr as return value if not found?
    bool NfTokensSingleProtocolManager::GetNfToken(const uint256 & tokenId, NfToken & nfToken) const
    {
        assert(!tokenId.IsNull());

        const auto nfTokenIt = m_nfTokensSet.find(tokenId);
        if (nfTokenIt == m_nfTokensSet.end())
        {
            return false;
        }

        nfToken = *nfTokenIt;
        return true;
    }


    bool NfTokensSingleProtocolManager::OwnerOf(const uint256 & tokenId, CKeyID & ownerId) const
    {
        assert(!tokenId.IsNull());

        const auto nfTokenIt = m_nfTokensSet.find(tokenId);
        if (nfTokenIt == m_nfTokensSet.end())
        {
            return false;
        }

        ownerId = nfTokenIt->tokenOwnerKeyId;
        return true;
    }

    std::size_t NfTokensSingleProtocolManager::BalanceOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NfTokenSetByOwnerId & ownerIdIndex = m_nfTokensSet.get<Tags::OwnerId>();
        return ownerIdIndex.count(ownerId);
    }

    std::vector<NfToken> NfTokensSingleProtocolManager::NfTokensOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NfTokenSetByOwnerId & ownerIdIndex = m_nfTokensSet.get<Tags::OwnerId>();
        const auto nfTokensRange = ownerIdIndex.equal_range(ownerId);

        std::vector<NfToken> nfTokens;
        std::for_each(nfTokensRange.first, nfTokensRange.second, std::back_inserter(nfTokens));
        return nfTokens;
    }

    std::vector<uint256> NfTokensSingleProtocolManager::NfTokenIdsOf(const CKeyID & ownerId) const
    {
        assert(!ownerId.IsNull());

        const NfTokenSetByOwnerId & ownerIdIndex = m_nfTokensSet.get<Tags::OwnerId>();
        const auto nfTokensRange = ownerIdIndex.equal_range(ownerId);

        std::vector<uint256> nfTokenIds;
        std::for_each(nfTokensRange.first, nfTokensRange.second, [](const NfToken & nfToken)
        {
            nfTokenIds.push_back(nfToken.tokenId);
        });
        return nfTokenIds;
    }

    std::size_t NfTokensSingleProtocolManager::TotalSupply() const
    {
        return m_nfTokensSet.size();
    }
}
