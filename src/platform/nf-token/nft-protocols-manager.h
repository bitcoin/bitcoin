// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROJECT_NFT_PROTOCOLS_MANAGER_H
#define PROJECT_NFT_PROTOCOLS_MANAGER_H

#include "nf-token-protocol-index.h"
#include "nf-token-multiindex-utils.h"
#include "chain.h"

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
    private:

    };
}

#endif //PROJECT_NFT_PROTOCOLS_MANAGER_H
