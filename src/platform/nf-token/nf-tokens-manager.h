// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKENS_MANAGER_H
#define CROWN_PLATFORM_NF_TOKENS_MANAGER_H

#include "nf-token-multiindex-utils.h"
#include "nf-token.h"

class CTransaction;
class CBlockIndex;

namespace Platform
{
    class NfTokenIndex
    {
    public:
        int height;
        //TODO: use block ptr for opt
        uint256 blockHash;
        uint256 regTxHash;
        std::shared_ptr<const NfToken> nfToken;
    };

    struct TokenProtocolIdExtractor
    {
        using result_type = uint64_t;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.nfToken->tokenProtocolId;
        }
    };

    struct TokenIdExtractor
    {
        using result_type = uint256;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.nfToken->tokenId;
        }
    };

    struct OwnerIdExtractor
    {
        using result_type = CKeyID;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.nfToken->tokenOwnerKeyId;
        }
    };

    struct AdminIdExtractor
    {
        using result_type = CKeyID;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.nfToken->metadataAdminKeyId;
        }
    };

    using NfTokensIndexSet = bmx::multi_index_container<
        NfTokenIndex,
        bmx::indexed_by<
            /// orderered by a composite-key <TokenProtocolId, TokenId>
            /// gives access to a globally unique nf token
            bmx::hashed_unique<
                bmx::tag<Tags::ProtocolIdTokenId>,
                bmx::composite_key<
                    NfTokenIndex,
                    TokenProtocolIdExtractor,
                    TokenIdExtractor
                >
            >,
            /// ordered by nf-token registration transaction hash
            /// gives access to nf-token registered in this transaction
            bmx::hashed_unique<
                bmx::tag<Tags::RegTxHash>,
                bmx::member<NfTokenIndex, uint256, &NfTokenIndex::regTxHash>
            >,
            /// ordered by nf-token registration block hash
            /// gives access to all nf-tokens registered in this block
            bmx::hashed_non_unique<
                bmx::tag<Tags::BlockHash>,
                bmx::member<NfTokenIndex, uint256, &NfTokenIndex::blockHash>
            >,
            /// ordered by a composity-key <TokenProtocolId, OwnerId>
            /// gives access to all nf-tokens owned by the OwnerId in this protocol
            bmx::hashed_non_unique<
                bmx::tag<Tags::ProtocolIdOwnerId>,
                bmx::composite_key<
                    NfTokenIndex,
                    TokenProtocolIdExtractor,
                    OwnerIdExtractor
                >
            >,
            /// ordered by nf-token protocol id
            /// gives access to all nf-tokens for this protocol
            bmx::hashed_non_unique<
                bmx::tag<Tags::ProtocolId>,
                TokenProtocolIdExtractor
            >,
            /// ordered by the OwnerId in the gloabal nf-tokens set
            /// gives access the global set of nf-tokens owned by the OwnerId
            bmx::hashed_non_unique<
                bmx::tag<Tags::OwnerId>,
                OwnerIdExtractor
            >
        >
    >;

    class NfTokensManager
    {
        public:
            static NfTokensManager & Instance()
            {
                if (s_instance.get() == nullptr)
                {
                    s_instance.reset(new NfTokensManager());
                }
                return *s_instance;
            }

            //std::unique_ptr<NfTokensSingleProtocolManager> ComposeSingleProtocolManager(const uint64_t & protocolId);

            /// Adds a new nf-token to the global set
            bool AddNfToken(const NfToken & nfToken, const CTransaction & tx, const CBlockIndex * pindex);

            /// Checks the existence of the specified nf-token
            bool Contains(const uint64_t & protocolId, const uint256 & tokenId) const;
            /// Checks the existence of the specified nf-token at the specified height
            bool ContainsAtHeight(const uint64_t & protocolId, const uint256 & tokenId, int height) const;

            /// Retreive the specified nf-token index
            const NfTokenIndex * GetNfTokenIndex(const uint64_t & protocolId, const uint256 & tokenId) const;
            /// Retreive the specified nf-token
            std::weak_ptr<const NfToken> GetNfToken(const uint64_t & protocolId, const uint256 & tokenId) const;

            /// Owner of the specified nf-token
            CKeyID OwnerOf(const uint64_t & protocolId, const uint256 & tokenId) const;

            /// Amount of all nf-tokens belonging to the specified owner within the protocol
            std::size_t BalanceOf(const uint64_t & protocolId, const CKeyID & ownerId) const;
            /// Retreive all nf-tokens belonging to the specified owner within the protocol
            std::vector<std::weak_ptr<const NfToken> > NfTokensOf(const uint64_t & protocolId, const CKeyID & ownerId) const;
            /// Retreive all nf-token IDs belonging to the specified owner within the protocol
            std::vector<uint256> NfTokenIdsOf(const uint64_t & protocolId, const CKeyID & ownerId) const;

            /// Amount of all nf-tokens belonging to the specified owner in the global protocol set
            std::size_t BalanceOf(const CKeyID & ownerId);
            /// Retreive all nf-tokens belonging to the specified owner in the global protocol set
            std::vector<std::weak_ptr<const NfToken> > NfTokensOf(const CKeyID & ownerId) const;
            /// Retreive all nf-tokens IDs belonging to the specified owner in the global protocol set
            std::vector<uint256> NfTokenIdsOf(const CKeyID & ownerId) const;

            /// Total amount of nf-tokens
            std::size_t TotalSupply() const;
            /// Total amount of nf-tokens for the specified protocol
            std::size_t TotalSupply(const uint64_t & protocolId) const;

        private:
            NfTokensIndexSet m_nfTokensIndexSet;

            static std::unique_ptr<NfTokensManager> s_instance;
    };

}

#endif // CROWN_PLATFORM_NF_TOKENS_MANAGER_H
