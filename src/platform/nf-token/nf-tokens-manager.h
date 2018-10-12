// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKENS_MANAGER_H
#define CROWN_PLATFORM_NF_TOKENS_MANAGER_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include "nf-token.h"
#include "nf-tokens-single-protocol-manager.h"

class CTransaction;
class CBlockIndex;

namespace Platform
{
    namespace bmx = boost::multi_index;

    namespace Tags
    {
        class BlockHash {};
        class RegTxHash {};
        class ProtocolId {};
        //class TokenId {};
        class ProtocolIdTokenId {};
        class ProtocolIdOwnerId {};
        //class OwnerId {};
        //class AdminId {};
    }

    class NfTokenIndex
    {
    public:
        //TODO: use block ptr for opt
        uint256 blockHash;
        int height;
        uint256 regTxHash;
        std::shared_ptr<const NfToken> nfToken;
    };

    struct TokenProtocolIdExtractor
    {
        using result_type = uint64_t;

        const result_type & operator()(const NfTokenIndex & nfTokenIndex)
        {
            return nfTokenIndex.nfToken->tokenProtocolId;
        }
    };

    struct TokenIdExtractor
    {
        using result_type = uint256;

        const result_type & operator()(const NfTokenIndex & nfTokenIndex)
        {
            return nfTokenIndex.nfToken->tokenId;
        }
    };

    struct OwnerIdExtractor
    {
        using result_type = CKeyID;

        const result_type & operator()(const NfTokenIndex & nfTokenIndex)
        {
            return nfTokenIndex.nfToken->tokenOwnerKeyId;
        }
    };

    struct AdminIdExtractor
    {
        using result_type = CKeyID;

        const result_type & operator()(const NfTokenIndex & nfTokenIndex)
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
                bmx::composite_key<
                    bmx::tag<Tags::ProtocolIdTokenId>,
                    TokenProtocolIdExtractor,
                    //bmx::member<NfTokenIndex, uint64_t, &NfToken::tokenProtocolId>,
                    TokenIdExtractor
                    //bmx::member<NfTokenIndex, uint256, &NfToken::tokenId>
                >
            >,
            /// ordered by nf-token registration transaction hash
            /// gives access to all nf-tokens registered in this transactions
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
                bmx::composite_key<
                    bmx::tag<Tags::ProtocolIdOwnerId>,
                    TokenProtocolIdExtractor,
                    //bmx::member<NfTokenIndex, uint64_t, &NfToken::tokenProtocolId>,
                    OwnerIdExtractor
                    //bmx::member<NfTokenIndex, CKeyID, &NfToken::tokenOwnerKeyId>
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
                    //bmx::member<NfTokenIndex, CKeyID, &NfToken::tokenOwnerKeyId>
            >
        >
    >;

    using NfTokenEntireSet = bmx::multi_index_container<
        NfToken,
        bmx::indexed_by<
            bmx::hashed_unique<
                bmx::composite_key<
                    bmx::tag<Tags::ProtocolIdTokenId>,
                    bmx::member<NfToken, uint64_t, &NfToken::tokenProtocolId>,
                    bmx::member<NfToken, uint256, &NfToken::tokenId>
                >
            >,
            bmx::hashed_non_unique<
                bmx::composite_key<
                    bmx::tag<Tags::ProtocolIdOwnerId>,
                    bmx::member<NfToken, uint64_t, &NfToken::tokenProtocolId>,
                    bmx::member<NfToken, CKeyID, &NfToken::tokenOwnerKeyId>
                >
            >,
            bmx::hashed_non_unique<
                bmx::tag<Tags::ProtocolId>,
                bmx::member<NfToken, uint64_t, &NfToken::tokenProtocolId>
            >,
            bmx::hashed_non_unique<
                    bmx::tag<Tags::OwnerId>,
                    bmx::member<NfToken, CKeyID, &NfToken::tokenOwnerKeyId>
            >//,
            //bmx::hashed_non_unique<
            //        bmx::tag<Tags::AdminId>,
            //        bmx::member<NfToken, CKeyID, &NfToken::metadataAdminKeyId>
            //>
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
            const NfTokenIndex & GetNfTokenIndex(const uint64_t & protocolId, const uint256 & tokenId) const;
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
