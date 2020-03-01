// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_NF_TOKENS_MANAGER_H
#define CROWN_PLATFORM_NF_TOKENS_MANAGER_H

#include <unordered_map>
#include <boost/range/adaptors.hpp>
#include <boost/range/any_range.hpp>

#include "sync.h"
#include "chain.h"
#include "nf-token-multiindex-utils.h"
#include "nf-token-index.h"

class CTransaction;
class CBlockIndex;

namespace Platform
{
    struct RegTxHashExtractor
    {
        using result_type = uint256;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.RegTxHash();
        }
    };

    struct TokenProtocolIdExtractor
    {
        using result_type = uint64_t;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.NfTokenPtr()->tokenProtocolId;
        }
    };

    struct TokenIdExtractor
    {
        using result_type = uint256;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.NfTokenPtr()->tokenId;
        }
    };

    struct OwnerIdExtractor
    {
        using result_type = CKeyID;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.NfTokenPtr()->tokenOwnerKeyId;
        }
    };

    struct AdminIdExtractor
    {
        using result_type = CKeyID;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.NfTokenPtr()->metadataAdminKeyId;
        }
    };

    struct BlockHashExtractor
    {
        using result_type = uint256;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return *nfTokenIndex.BlockIndex()->phashBlock;
        }
    };

    struct HeightExtractor
    {
        using result_type = int;

        result_type operator()(const NfTokenIndex & nfTokenIndex) const
        {
            return nfTokenIndex.BlockIndex()->nHeight;
        }
    };

    using NfTokensIndexSet = bmx::multi_index_container<
        NfTokenIndex,
        bmx::indexed_by<
            /// hash-indexed by a composite-key <TokenProtocolId, TokenId>
            /// gives access to a globally unique nf token
            bmx::hashed_unique<
                bmx::tag<Tags::ProtocolIdTokenId>,
                bmx::composite_key<
                    NfTokenIndex,
                    TokenProtocolIdExtractor,
                    TokenIdExtractor
                >
            >,
            /// hash-indexed by nf-token registration transaction hash
            /// gives access to the nf-token registered in a specified transaction
            bmx::hashed_unique<
                bmx::tag<Tags::RegTxHash>,
                RegTxHashExtractor
            >,
            /// hash-indexed by nf-token registration block hash
            /// gives access to all nf-tokens registered in a specified block
            bmx::hashed_non_unique<
                bmx::tag<Tags::BlockHash>,
                BlockHashExtractor
            >,
            /// ordered by nf-token registration block height
            /// gives access to all nf-tokens registered at a specific block height
            /// or gives access to a range requested by height
            bmx::ordered_non_unique<
                bmx::tag<Tags::Height>,
                HeightExtractor
            >,
            /// ordered by nf-token protocol id and registration block height
            /// gives access to all nf-tokens registered at a specific block height
            /// or gives access to a range requested by height
            bmx::ordered_non_unique<
                bmx::tag<Tags::ProtocolIdHeight>,
                bmx::composite_key<
                    NfTokenIndex,
                    TokenProtocolIdExtractor,
                    HeightExtractor
                >
            >,
            /// hash-indexed by a composite-key <TokenProtocolId, OwnerId>
            /// gives access to all nf-tokens owned by the OwnerId in a specified protocol
            bmx::ordered_non_unique<
                bmx::tag<Tags::ProtocolIdOwnerId>,
                bmx::composite_key<
                    NfTokenIndex,
                    TokenProtocolIdExtractor,
                    OwnerIdExtractor,
                    HeightExtractor
                >
            >,
            /// hash-indexed by nf-token protocol id
            /// gives access to all nf-tokens for a specified protocol
            bmx::hashed_non_unique<
                bmx::tag<Tags::ProtocolId>,
                TokenProtocolIdExtractor
            >,
            /// hash-indexed by the OwnerId in the global nf-tokens set
            /// gives access a global set of nf-tokens owned by the OwnerId
            bmx::ordered_non_unique<
                bmx::tag<Tags::OwnerId>,
                bmx::composite_key<
                    NfTokenIndex,
                    OwnerIdExtractor,
                    HeightExtractor
                >
            >
        >
    >;

    class NfTokensManager
    {
        public:
            static NfTokensManager & Instance()
            {
                if (s_instance == nullptr)
                {
                    s_instance.reset(new NfTokensManager());
                }
                return *s_instance;
            }

            /// Adds a new nf-token to the global set
            bool AddNfToken(const NfToken & nfToken, const CTransaction & tx, const CBlockIndex * pindex);

            /// Checks the existence of a specified nf-token
            bool Contains(uint64_t protocolId, const uint256 & tokenId);
            /// Checks the existence of a specified nf-token at a specified height
            bool Contains(uint64_t protocolId, const uint256 & tokenId, int height);

            /// Retrieve a specified nf-token index by a protocol ID and token ID, may be null
            NfTokenIndex GetNfTokenIndex(uint64_t protocolId, const uint256 & tokenId);
            /// Retrieve a specified nf-token index by a transaction ID, may be null
            NfTokenIndex GetNfTokenIndex(const uint256 & regTxId);

            /// Owner of a specified nf-token
            CKeyID OwnerOf(uint64_t protocolId, const uint256 & tokenId);

            /// Amount of all nf-tokens belonging to a specified owner within a protocol
            std::size_t BalanceOf(uint64_t protocolId, const CKeyID & ownerId) const;
            /// Amount of all nf-tokens belonging to a specified owner in a global protocol set
            std::size_t BalanceOf(const CKeyID & ownerId) const;

            /// Retrieve all nf-tokens belonging to a specified owner within a protocol
            std::vector<std::weak_ptr<const NfToken> > NfTokensOf(uint64_t protocolId, const CKeyID & ownerId) const;
            /// Retrieve all nf-tokens belonging to a specified owner in a global protocol set
            std::vector<std::weak_ptr<const NfToken> > NfTokensOf(const CKeyID & ownerId) const;

            /// Retrieve all nf-token IDs belonging to a specified owner within a protocol
            std::vector<uint256> NfTokenIdsOf(uint64_t protocolId, const CKeyID & ownerId) const;
            /// Retrieve all nf-tokens IDs belonging to a specified owner in a global protocol set
            std::vector<uint256> NfTokenIdsOf(const CKeyID & ownerId) const;

            /// Total amount of nf-tokens
            std::size_t TotalSupply() const;
            /// Total amount of nf-tokens for a specified protocol
            std::size_t TotalSupply(uint64_t protocolId) const;

            using NftIndexRange = boost::any_range<const NfTokenIndex &, boost::bidirectional_traversal_tag>;
            using NftIndexForwardRange = boost::any_range<const NfTokenIndex &, boost::forward_traversal_tag>;

            void ProcessFullNftIndexRange(std::function<bool(const NfTokenIndex &)> nftIndexHandler) const;
            void ProcessNftIndexRangeByHeight(std::function<bool(const NfTokenIndex &)> nftIndexHandler,
                                              unsigned int height,
                                              unsigned int count,
                                              unsigned int skipFromTip) const;

            void ProcessNftIndexRangeByHeight(std::function<bool(const NfTokenIndex &)> nftIndexHandler,
                                              uint64_t nftProtoId,
                                              unsigned int height,
                                              unsigned int count,
                                              unsigned int skipFromTip) const;

            void ProcessNftIndexRangeByHeight(std::function<bool(const NfTokenIndex &)> nftIndexHandler,
                                              CKeyID keyId,
                                              unsigned int height,
                                              unsigned int count,
                                              unsigned int skipFromTip) const;

            void ProcessNftIndexRangeByHeight(std::function<bool(const NfTokenIndex &)> nftIndexHandler,
                                              uint64_t nftProtoId,
                                              CKeyID keyId,
                                              unsigned int height,
                                              unsigned int count,
                                              unsigned int skipFromTip) const;

            /// Delete a specified nf-token
            bool Delete(uint64_t protocolId, const uint256 & tokenId);
            /// Delete a specified nf-token at a specified block height, ignore if at different height
            bool Delete(uint64_t protocolId, const uint256 & tokenId, int height);

            /// Update with the best block tip
            void UpdateBlockTip(const CBlockIndex * pindex);

            /// Add new registered NFT protocol
            void OnNewProtocolRegistered(uint64_t protocolId);

        private:
            NfTokensManager();

            void UpdateTotalSupply(uint64_t protocolId, bool increase);
            NfTokenIndex GetNftIndexFromDb(uint64_t protocolId, const uint256 & tokenId);

        private:
            NfTokensIndexSet m_nfTokensIndexSet;
            int m_tipHeight{-1};
            uint256 m_tipBlockHash;
            mutable CCriticalSection m_cs;

            std::unordered_map<uint64_t, std::size_t> m_protocolsTotalSupply;

            static std::unique_ptr<NfTokensManager> s_instance;
    };

}

#endif // CROWN_PLATFORM_NF_TOKENS_MANAGER_H
