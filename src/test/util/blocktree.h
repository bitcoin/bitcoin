// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCKTREE_H
#define BITCOIN_TEST_UTIL_BLOCKTREE_H

#include <arith_uint256.h>
#include <chain.h>
#include <kernel/cs_main.h>
#include <primitives/block.h>
#include <sync.h>
#include <uint256.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

/** Options for the test blocks built by MakeHeader() and BlockTree::Add(). */
struct BlockTreeAddOptions {
    //! Mark the block as stored and validated up to BLOCK_VALID_TRANSACTIONS.
    bool have_data{false};
    uint32_t bits{0x1d00ffff};
    //! Merkle root of the header, or a value unique to the block if unset.
    std::optional<uint256> merkle_root{};
};

/** Build a header on `prev` whose hash is unique for each `nonce`. */
inline CBlockHeader MakeHeader(const CBlockIndex* prev, uint32_t nonce, const BlockTreeAddOptions& opts)
{
    CBlockHeader header;
    header.nVersion = 4;
    header.hashPrevBlock = prev ? prev->GetBlockHash() : uint256::ZERO;
    header.hashMerkleRoot = opts.merkle_root.value_or(uint256{static_cast<uint8_t>(nonce)});
    header.nTime = 1'700'000'000 + nonce;
    header.nBits = opts.bits;
    header.nNonce = nonce;
    return header;
}

/** Block index entries linked into a tree without a BlockManager, for unit
 *  tests of code that walks the block index. */
struct BlockTree
{
    //! A deque so phashBlock pointers stay valid as blocks are added.
    std::deque<uint256> hashes;
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    CChain active_chain;
    uint32_t next_nonce{1};

    /** Add a block on top of `prev`, or a genesis block if `prev` is null.
     *  If `active`, make it the active tip. */
    CBlockIndex* Add(CBlockIndex* prev, bool active, BlockTreeAddOptions opts = {}) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        const CBlockHeader header{MakeHeader(prev, next_nonce++, opts)};

        hashes.push_back(header.GetHash());
        auto block{std::make_unique<CBlockIndex>(header)};
        block->phashBlock = &hashes.back();
        block->pprev = prev;
        block->nHeight = prev ? prev->nHeight + 1 : 0;
        block->nChainWork = (prev ? prev->nChainWork : arith_uint256{}) + arith_uint256{1};
        block->nStatus = BLOCK_VALID_TREE;
        if (opts.have_data) {
            block->RaiseValidity(BLOCK_VALID_TRANSACTIONS);
            block->nStatus |= BLOCK_HAVE_DATA;
        }
        block->BuildSkip();
        CBlockIndex* ret{block.get()};
        blocks.push_back(std::move(block));
        if (active) active_chain.SetTip(*ret);
        return ret;
    }

    /** Add a genesis block and `height` more blocks on top of it, all on the
     *  active chain, and return the tip. */
    CBlockIndex* AddChain(int height, BlockTreeAddOptions opts = {}) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        CBlockIndex* tip{Add(nullptr, /*active=*/true, opts)};
        for (int i{0}; i < height; ++i) tip = Add(tip, /*active=*/true, opts);
        return tip;
    }
};

#endif // BITCOIN_TEST_UTIL_BLOCKTREE_H
