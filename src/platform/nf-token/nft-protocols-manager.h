// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROJECT_NFT_PROTOCOLS_MANAGER_H
#define PROJECT_NFT_PROTOCOLS_MANAGER_H

#include <unordered_map>
#include <boost/range/adaptors.hpp>
#include <boost/range/any_range.hpp>

#include "sync.h"
#include "chain.h"
#include "nf-token-multiindex-utils.h"
#include "nf-token-protocol-index.h"

namespace Platform
{
    struct NftProtoIdExtractor
    {
        using result_type = uint64_t;

        result_type operator()(const NftProtoIndex & nftProtoIndex) const
        {
            return nftProtoIndex.NftProtoPtr()->tokenProtocolId;
        }
    };

    struct NftProtoRegTxHashExtractor
    {
        using result_type = uint256;

        result_type operator()(const NftProtoIndex & nftProtoIndex) const
        {
            return nftProtoIndex.RegTxHash();
        }
    };

    struct NftProtoHeightExtractor
    {
        using result_type = int;

        result_type operator()(const NftProtoIndex & nftProtoIndex) const
        {
            return nftProtoIndex.BlockIndex()->nHeight;
        }
    };

    using NftProtosIndexSet = bmx::multi_index_container<
            NftProtoIndex,
            bmx::indexed_by<
                /// hash-indexed by NFT protocol ID
                /// gives access to a globally unique NFT protocol
                bmx::hashed_unique<
                    bmx::tag<Tags::ProtocolId>,
                    NftProtoIdExtractor
                >,
                /// hash-indexed by NFT protocol registration transaction hash
                /// gives access to the NFT protocol  registered in a specified transaction
                bmx::hashed_unique<
                    bmx::tag<Tags::RegTxHash>,
                    NftProtoRegTxHashExtractor
                >,
                /// ordered by NFT protocol registration block height
                /// gives access to all NFT protocols registered at a specific block height
                /// or gives access to a range requested by height
                bmx::ordered_non_unique<
                    bmx::tag<Tags::Height>,
                    NftProtoHeightExtractor
                >
            >
        >;

    class NftProtocolsManager
    {
    public:
        static NftProtocolsManager & Instance()
        {
            if (s_instance == nullptr)
            {
                s_instance.reset(new NftProtocolsManager());
            }
            return *s_instance;
        }

        /// Adds a new nf-token protocol to the global set
        bool AddNftProto(const NfTokenProtocol & nfTokenProto, const CTransaction & tx, const CBlockIndex * pindex);

        /// Checks the existence of a specified nf-token protocol
        bool Contains(uint64_t protocolId);
        /// Checks the existence of a specified nf-token protocol at a specified height
        bool Contains(uint64_t protocolId, int height);

        /// Retrieve a specified nf-token proto index by a protocol ID, may be null
        NftProtoIndex GetNftProtoIndex(uint64_t protocolId);
        /// Retrieve a specified nf-token proto index by a transaction ID, may be null
        NftProtoIndex GetNftProtoIndex(const uint256 & regTxId);

        /// Owner of a specified nf-token protocol
        CKeyID OwnerOf(uint64_t protocolId);

        /// Retrieve total amount of nft protocols
        std::size_t TotalSupply() const;

        using NftProtoIndexRange = boost::any_range<const NftProtoIndex &, boost::bidirectional_traversal_tag>;

        void ProcessFullNftProtoIndexRange(std::function<bool(const NftProtoIndex &)> protoIndexHandler) const;
        void ProcessNftProtoIndexRangeByHeight(std::function<bool(const NftProtoIndex &)> protoIndexHandler,
                                          unsigned int height,
                                          unsigned int count,
                                          unsigned int skipFromTip) const;

        /// Delete a specified nf-token protocol
        bool Delete(uint64_t protocolId);
        /// Delete a specified nf-token protocol at a specified block height, ignore if at different height
        bool Delete(uint64_t protocolId, int height);

        /// Update with the best block tip
        void UpdateBlockTip(const CBlockIndex * pindex);

    private:
        NftProtocolsManager();
        /// Reserved for future use
        /// NftProtoIndex GetNftProtoIndexFromDb(uint64_t protocolId);

    private:
        NftProtosIndexSet m_nftProtoIndexSet;
        int m_tipHeight{-1};
        uint256 m_tipBlockHash;
        mutable CCriticalSection m_cs;

        std::size_t m_totalProtocolsCount{0};

        static std::unique_ptr<NftProtocolsManager> s_instance;
    };
}

#endif //PROJECT_NFT_PROTOCOLS_MANAGER_H
