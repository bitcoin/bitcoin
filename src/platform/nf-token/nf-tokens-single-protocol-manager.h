// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKENS_SINGLE_PROTOCOL_MANAGER_H
#define CROWN_PLATFORM_NF_TOKENS_SINGLE_PROTOCOL_MANAGER_H

#include "nf-token-multiindex-utils.h"
#include "nf-token.h"

namespace Platform
{
    using NfTokenSingleProtocolSet = bmx::multi_index_container<
        NfToken,
        bmx::indexed_by<
            bmx::hashed_unique<
                bmx::tag<Tags::TokenId>,
                bmx::member<NfToken, uint256, &NfToken::tokenId>
            >,
            bmx::hashed_non_unique<
                bmx::tag<Tags::OwnerId>,
                bmx::member<NfToken, CKeyID, &NfToken::tokenOwnerKeyId>
            >//,
            //bmx::hashed_non_unique<
            //    bmx::tag<Tags::AdminId>,
            //    bmx::member<NfToken, CKeyID, &NfToken::metadataAdminKeyId>
            //>
        >
    >;

    class NfTokensSingleProtocolManager
    {
    public:
        explicit NfTokensSingleProtocolManager(const uint64_t & protocolId);

        //std::string ProtocolSymbol() const;
        //std::string ProtocolName() const;
        uint64_t GetProtocolId() const;

        bool AddNfToken(NfToken nfToken);

        bool Contains(const uint256 & tokenId) const;
        bool GetNfToken(const uint256 & tokenId, NfToken & nfToken) const;
        bool OwnerOf(const uint256 & tokenId, CKeyID & ownerId) const;

        std::size_t BalanceOf(const CKeyID & ownerId) const;
        std::vector<NfToken> NfTokensOf(const CKeyID & ownerId) const;
        std::vector<uint256> NfTokenIdsOf(const CKeyID & ownerId) const;

        std::size_t TotalSupply() const;

    private:
        uint64_t m_protocolId;
        NfTokenSingleProtocolSet m_nfTokensSet;
    };

}

#endif // CROWN_PLATFORM_NF_TOKENS_SINGLE_PROTOCOL_MANAGER_H
