// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/timestampindex.h>

#include <chain.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/system.h>
#include <validation.h>

constexpr uint8_t DB_TIMESTAMPINDEX{'s'};

TimestampIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "timestampindex", n_cache_size, f_memory, f_wipe)
{
}

bool TimestampIndex::DB::Write(const CTimestampIndexKey& key)
{
    return CDBWrapper::Write(std::make_pair(DB_TIMESTAMPINDEX, key), true);
}

void TimestampIndex::DB::ReadRange(uint32_t high, uint32_t low, std::vector<uint256>& hashes)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    // Seek to the starting timestamp
    pcursor->Seek(std::make_pair(DB_TIMESTAMPINDEX, CTimestampIndexIteratorKey(low)));

    // Iterate through all entries in the timestamp range
    while (pcursor->Valid()) {
        std::pair<uint8_t, CTimestampIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_TIMESTAMPINDEX && key.second.m_block_time <= high) {
            hashes.push_back(key.second.m_block_hash);
            pcursor->Next();
        } else {
            break;
        }
    }
}

void TimestampIndex::DB::EraseTimestampIndex(CDBBatch& batch, const CTimestampIndexKey& key)
{
    batch.Erase(std::make_pair(DB_TIMESTAMPINDEX, key));
}

TimestampIndex::TimestampIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex(std::move(chain), "timestampindex"),
    m_db(std::make_unique<TimestampIndex::DB>(n_cache_size, f_memory, f_wipe))
{
}

TimestampIndex::~TimestampIndex() = default;

bool TimestampIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Skip genesis block
    if (block.height == 0) return true;

    // Create timestamp index key from block metadata
    CTimestampIndexKey key(block.data->nTime, block.hash);

    // Write to database
    return m_db->Write(key);
}

bool TimestampIndex::CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip)
{
    const CBlockIndex* current_tip_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(current_tip.hash));
    const CBlockIndex* new_tip_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(new_tip.hash));
    assert(current_tip_index->GetAncestor(new_tip_index->nHeight) == new_tip_index);

    // Erase timestamp index entries for blocks being rewound, in one batch with the locator
    CDBBatch batch(GetDB());
    for (const CBlockIndex* pindex = current_tip_index; pindex != new_tip_index; pindex = pindex->pprev) {
        // Skip genesis block
        if (pindex->nHeight == 0) continue;

        m_db->EraseTimestampIndex(batch, CTimestampIndexKey(pindex->nTime, pindex->GetBlockHash()));
    }

    WriteRewindLocator(batch, new_tip_index);
    return GetDB().WriteBatch(batch);
}

BaseIndex::DB& TimestampIndex::GetDB() const { return *m_db; }

void TimestampIndex::GetBlockHashes(uint32_t high, uint32_t low, std::vector<uint256>& hashes) const
{
    m_db->ReadRange(high, low, hashes);
}
