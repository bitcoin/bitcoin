// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <chain.h>
#include <kernel/blocktreestorage.h>
#include <kernel/cs_main.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

using namespace util::hex_literals;
using kernel::BlockTreeStore;
using kernel::CBlockFileInfo;

static std::vector<CBlockFileInfo> CreateUniqueFileInfo(int32_t count)
{
    std::vector<CBlockFileInfo> infos;
    kernel::CBlockFileInfo info;
    info.nBlocks = count;
    info.nSize = count + 1;
    info.nUndoSize = count + 2;
    info.nHeightFirst = count + 3;
    info.nHeightLast = count + 4;
    info.nTimeFirst = count + 5;
    info.nTimeLast = count + 6;
    infos.emplace_back(info);
    return infos;
}

static auto BuildFileInfo(std::span<CBlockFileInfo> infos)
{
    std::vector<std::pair<int, const CBlockFileInfo*>> file_info;
    file_info.reserve(infos.size());
    for (uint32_t i = 0; i < infos.size(); ++i) {
        file_info.emplace_back(i, &infos[i]);
    }
    return file_info;
}

static void BuildBlockIndex(node::BlockMap& block_map)
{
    LOCK(cs_main);
    CBlockIndex* prev{nullptr};
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256{};
    header.hashMerkleRoot = uint256{};
    header.nTime = 0;
    header.nBits = 0;
    header.nNonce = 0;
    const auto [it, inserted]{block_map.try_emplace(header.GetHash(), header)};
    assert(inserted); // unique nNonce/nTime/hashPrev => unique hash
    CBlockIndex* pindex{&it->second};
    pindex->phashBlock = &it->first;
    pindex->pprev = prev;
    pindex->nHeight = 0;
    pindex->nStatus = BLOCK_HAVE_DATA;
    pindex->nTx = 1;
    pindex->nFile = 0;
    pindex->nDataPos = 0;
    pindex->nUndoPos = 0;
    prev = pindex;
}

static void WriteBlockIndexLevelDB(benchmark::Bench& bench)
{
    const auto setup{MakeNoLogFileContext<>(ChainType::MAIN)};
    auto path = setup->m_path_root;
    DBParams params{
        .path = path,
        .cache_bytes = 0,
        .memory_only = false,
        .wipe_data = true,
        .obfuscate = false,
    };
    kernel::BlockTreeDB block_tree_db{params};
    auto file_info = CreateUniqueFileInfo(0);
    auto file_info_pointers = BuildFileInfo(file_info);
    node::BlockMap block_map;
    std::vector<const CBlockIndex*> blocks;
    BuildBlockIndex(block_map);
    for (auto& entry : block_map) {
        blocks.push_back(&entry.second);
    }

    bench.run("leveldb", [&] {
        block_tree_db.WriteBatchSync(file_info_pointers, 1, blocks);
    });
}

static void WriteBlockIndexBlockTreeStore(benchmark::Bench& bench)
{
    const auto setup{MakeNoLogFileContext<>(ChainType::MAIN)};
    auto file_info = CreateUniqueFileInfo(0);
    auto file_info_pointers = BuildFileInfo(file_info);
    node::BlockMap block_map;
    std::vector<CBlockIndex*> blocks;
    BuildBlockIndex(block_map);
    for (auto& entry : block_map) {
        blocks.push_back(&entry.second);
    }

    auto path = setup->m_path_root;
    BlockTreeStore block_tree_store{path};

    bench.run("block_tree_store", [&] {
        LOCK(cs_main);
        block_tree_store.WriteBatchSync(file_info_pointers, blocks);
    });
}


BENCHMARK(WriteBlockIndexLevelDB);
BENCHMARK(WriteBlockIndexBlockTreeStore);
