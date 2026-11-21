// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <staletipmessage.h>

#include <uint256.h>

#include <vector>

std::vector<CBlockHeader> StaleTipMessage::DecompressHeaders() const
{
    std::vector<CBlockHeader> decompressed;
    decompressed.reserve(headers.size());

    uint256 prev_hash{fork_point};
    for (const auto& compressed : headers) {
        CBlockHeader header;
        header.nVersion = compressed.version;
        header.hashPrevBlock = prev_hash;
        header.hashMerkleRoot = compressed.merkle_root;
        header.nTime = compressed.time;
        header.nBits = compressed.bits;
        header.nNonce = compressed.nonce;
        decompressed.push_back(header);
        prev_hash = decompressed.back().GetHash();
    }

    return decompressed;
}
