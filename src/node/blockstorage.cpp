// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>

#include <arith_uint256.h>
#include <chain.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <hash.h>
#include <kernel/blockmanager_opts.h>
#include <kernel/chainparams.h>
#include <kernel/messagestartchars.h>
#include <kernel/notifications_interface.h>
#include <logging.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <signet.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <undo.h>
#include <util/batchpriority.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/obfuscation.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/syserror.h>
#include <util/translation.h>
#include <validation.h>

#include <cstddef>
#include <map>
#include <optional>
#include <unordered_map>

namespace kernel {
static constexpr uint8_t DB_BLOCK_FILES{'f'};
static constexpr uint8_t DB_BLOCK_INDEX{'b'};
static constexpr uint8_t DB_FLAG{'F'};
static constexpr uint8_t DB_REINDEX_FLAG{'R'};
static constexpr uint8_t DB_LAST_BLOCK{'l'};
// Keys used in previous version that might still be found in the DB:
// BlockTreeDB::DB_TXINDEX_BLOCK{'T'};
// BlockTreeDB::DB_TXINDEX{'t'}
// BlockTreeDB::ReadFlag("txindex")

bool BlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    return Read(std::make_pair(DB_BLOCK_FILES, nFile), info);
}

bool BlockTreeDB::WriteReindexing(bool fReindexing)
{
    if (fReindexing) {
        return Write(DB_REINDEX_FLAG, uint8_t{'1'});
    } else {
        return Erase(DB_REINDEX_FLAG);
    }
}

void BlockTreeDB::ReadReindexing(bool& fReindexing)
{
    fReindexing = Exists(DB_REINDEX_FLAG);
}

bool BlockTreeDB::ReadLastBlockFile(int& nFile)
{
    return Read(DB_LAST_BLOCK, nFile);
}

bool BlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*>>& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo)
{
    CDBBatch batch(*this);
    for (const auto& [file, info] : fileInfo) {
        batch.Write(std::make_pair(DB_BLOCK_FILES, file), *info);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (const CBlockIndex* bi : blockinfo) {
        batch.Write(std::make_pair(DB_BLOCK_INDEX, bi->GetBlockHash()), CDiskBlockIndex{bi});
    }
    return WriteBatch(batch, true);
}

bool BlockTreeDB::WriteFlag(const std::string& name, bool fValue)
{
    return Write(std::make_pair(DB_FLAG, name), fValue ? uint8_t{'1'} : uint8_t{'0'});
}

bool BlockTreeDB::ReadFlag(const std::string& name, bool& fValue)
{
    uint8_t ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch)) {
        return false;
    }
    fValue = ch == uint8_t{'1'};
    return true;
}

bool BlockTreeDB::LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex, const util::SignalInterrupt& interrupt)
{
    AssertLockHeld(::cs_main);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(DB_BLOCK_INDEX, uint256()));

    // Load m_block_index
    while (pcursor->Valid()) {
        if (interrupt) return false;
        std::pair<uint8_t, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                CBlockIndex* pindexNew = insertBlockIndex(diskindex.ConstructBlockHash());
                pindexNew->pprev          = insertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;

                if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits, consensusParams)) {
                    LogError("%s: CheckProofOfWork failed: %s\n", __func__, pindexNew->ToString());
                    return false;
                }

                pcursor->Next();
            } else {
                LogError("%s: failed to read value\n", __func__);
                return false;
            }
        } else {
            break;
        }
    }

    return true;
}
} // namespace kernel

namespace node {

bool CBlockIndexWorkComparator::operator()(const CBlockIndex* pa, const CBlockIndex* pb) const
{
    // First sort by most total work, ...
    if (pa->nChainWork > pb->nChainWork) return false;
    if (pa->nChainWork < pb->nChainWork) return true;

    // ... then by earliest activatable time, ...
    if (pa->nSequenceId < pb->nSequenceId) return false;
    if (pa->nSequenceId > pb->nSequenceId) return true;

    // Use pointer address as tie breaker (should only happen with blocks
    // loaded from disk, as those share the same id: 0 for blocks on the
    // best chain, 1 for all others).
    if (pa < pb) return false;
    if (pa > pb) return true;

    // Identical blocks.
    return false;
}

bool CBlockIndexHeightOnlyComparator::operator()(const CBlockIndex* pa, const CBlockIndex* pb) const
{
    return pa->nHeight < pb->nHeight;
}

std::vector<CBlockIndex*> BlockManager::GetAllBlockIndices()
{
    AssertLockHeld(cs_main);
    std::vector<CBlockIndex*> rv;
    rv.reserve(m_block_index.size());
    for (auto& [_, block_index] : m_block_index) {
        rv.push_back(&block_index);
    }
    return rv;
}

CBlockIndex* BlockManager::LookupBlockIndex(const uint256& hash)
{
    AssertLockHeld(cs_main);
    BlockMap::iterator it = m_block_index.find(hash);
    return it == m_block_index.end() ? nullptr : &it->second;
}

const CBlockIndex* BlockManager::LookupBlockIndex(const uint256& hash) const
{
    AssertLockHeld(cs_main);
    BlockMap::const_iterator it = m_block_index.find(hash);
    return it == m_block_index.end() ? nullptr : &it->second;
}

CBlockIndex* BlockManager::AddToBlockIndex(const CBlockHeader& block, CBlockIndex*& best_header)
{
    AssertLockHeld(cs_main);

    auto [mi, inserted] = m_block_index.try_emplace(block.GetHash(), block);
    if (!inserted) {
        return &mi->second;
    }
    CBlockIndex* pindexNew = &(*mi).second;

    // We assign the sequence id to blocks only when the full data is available,
    // to avoid miners withholding blocks but broadcasting headers, to get a
    // competitive advantage.
    pindexNew->nSequenceId = SEQ_ID_INIT_FROM_DISK;

    pindexNew->phashBlock = &((*mi).first);
    BlockMap::iterator miPrev = m_block_index.find(block.hashPrevBlock);
    if (miPrev != m_block_index.end()) {
        pindexNew->pprev = &(*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
        pindexNew->BuildSkip();
    }
    pindexNew->nTimeMax = (pindexNew->pprev ? std::max(pindexNew->pprev->nTimeMax, pindexNew->nTime) : pindexNew->nTime);
    pindexNew->nChainWork = (pindexNew->pprev ? pindexNew->pprev->nChainWork : 0) + GetBlockProof(*pindexNew);
    pindexNew->RaiseValidity(BLOCK_VALID_TREE);
    if (best_header == nullptr || best_header->nChainWork < pindexNew->nChainWork) {
        best_header = pindexNew;
    }

    m_dirty_blockindex.insert(pindexNew);

    return pindexNew;
}

void BlockManager::PruneOneBlockFile(const int fileNumber)
{
    AssertLockHeld(cs_main);
    LOCK(cs_LastBlockFile);

    for (auto& entry : m_block_index) {
        CBlockIndex* pindex = &entry.second;
        if (pindex->nFile == fileNumber) {
            pindex->nStatus &= ~BLOCK_HAVE_DATA;
            pindex->nStatus &= ~BLOCK_HAVE_UNDO;
            pindex->nFile = 0;
            pindex->nDataPos = 0;
            pindex->nUndoPos = 0;
            m_dirty_blockindex.insert(pindex);

            // Prune from m_blocks_unlinked -- any block we prune would have
            // to be downloaded again in order to consider its chain, at which
            // point it would be considered as a candidate for
            // m_blocks_unlinked or setBlockIndexCandidates.
            auto range = m_blocks_unlinked.equal_range(pindex->pprev);
            while (range.first != range.second) {
                std::multimap<CBlockIndex*, CBlockIndex*>::iterator _it = range.first;
                range.first++;
                if (_it->second == pindex) {
                    m_blocks_unlinked.erase(_it);
                }
            }
        }
    }

    m_blockfile_info.at(fileNumber) = CBlockFileInfo{};
    m_dirty_fileinfo.insert(fileNumber);
}

void BlockManager::FindFilesToPruneManual(
    std::set<int>& setFilesToPrune,
    int nManualPruneHeight,
    const Chainstate& chain,
    ChainstateManager& chainman)
{
    assert(IsPruneMode() && nManualPruneHeight > 0);

    LOCK2(cs_main, cs_LastBlockFile);
    if (chain.m_chain.Height() < 0) {
        return;
    }

    const auto [min_block_to_prune, last_block_can_prune] = chainman.GetPruneRange(chain, nManualPruneHeight);

    int count = 0;
    for (int fileNumber = 0; fileNumber < this->MaxBlockfileNum(); fileNumber++) {
        const auto& fileinfo = m_blockfile_info[fileNumber];
        if (fileinfo.nSize == 0 || fileinfo.nHeightLast > (unsigned)last_block_can_prune || fileinfo.nHeightFirst < (unsigned)min_block_to_prune) {
            continue;
        }

        PruneOneBlockFile(fileNumber);
        setFilesToPrune.insert(fileNumber);
        count++;
    }
    LogInfo("[%s] Prune (Manual): prune_height=%d removed %d blk/rev pairs",
        chain.GetRole(), last_block_can_prune, count);
}

void BlockManager::FindFilesToPrune(
    std::set<int>& setFilesToPrune,
    int last_prune,
    const Chainstate& chain,
    ChainstateManager& chainman)
{
    LOCK2(cs_main, cs_LastBlockFile);
    // Distribute our -prune budget over all chainstates.
    const auto target = std::max(
        MIN_DISK_SPACE_FOR_BLOCK_FILES, GetPruneTarget() / chainman.GetAll().size());
    const uint64_t target_sync_height = chainman.m_best_header->nHeight;

    if (chain.m_chain.Height() < 0 || target == 0) {
        return;
    }
    if (static_cast<uint64_t>(chain.m_chain.Height()) <= chainman.GetParams().PruneAfterHeight()) {
        return;
    }

    const auto [min_block_to_prune, last_block_can_prune] = chainman.GetPruneRange(chain, last_prune);

    uint64_t nCurrentUsage = CalculateCurrentUsage();
    // We don't check to prune until after we've allocated new space for files
    // So we should leave a buffer under our target to account for another allocation
    // before the next pruning.
    uint64_t nBuffer = BLOCKFILE_CHUNK_SIZE + UNDOFILE_CHUNK_SIZE;
    uint64_t nBytesToPrune;
    int count = 0;

    if (nCurrentUsage + nBuffer >= target) {
        // On a prune event, the chainstate DB is flushed.
        // To avoid excessive prune events negating the benefit of high dbcache
        // values, we should not prune too rapidly.
        // So when pruning in IBD, increase the buffer to avoid a re-prune too soon.
        const auto chain_tip_height = chain.m_chain.Height();
        if (chainman.IsInitialBlockDownload() && target_sync_height > (uint64_t)chain_tip_height) {
            // Since this is only relevant during IBD, we assume blocks are at least 1 MB on average
            static constexpr uint64_t average_block_size = 1000000;  /* 1 MB */
            const uint64_t remaining_blocks = target_sync_height - chain_tip_height;
            nBuffer += average_block_size * remaining_blocks;
        }

        for (int fileNumber = 0; fileNumber < this->MaxBlockfileNum(); fileNumber++) {
            const auto& fileinfo = m_blockfile_info[fileNumber];
            nBytesToPrune = fileinfo.nSize + fileinfo.nUndoSize;

            if (fileinfo.nSize == 0) {
                continue;
            }

            if (nCurrentUsage + nBuffer < target) { // are we below our target?
                break;
            }

            // don't prune files that could have a block that's not within the allowable
            // prune range for the chain being pruned.
            if (fileinfo.nHeightLast > (unsigned)last_block_can_prune || fileinfo.nHeightFirst < (unsigned)min_block_to_prune) {
                continue;
            }

            PruneOneBlockFile(fileNumber);
            // Queue up the files for removal
            setFilesToPrune.insert(fileNumber);
            nCurrentUsage -= nBytesToPrune;
            count++;
        }
    }

    LogDebug(BCLog::PRUNE, "[%s] target=%dMiB actual=%dMiB diff=%dMiB min_height=%d max_prune_height=%d removed %d blk/rev pairs\n",
             chain.GetRole(), target / 1024 / 1024, nCurrentUsage / 1024 / 1024,
             (int64_t(target) - int64_t(nCurrentUsage)) / 1024 / 1024,
             min_block_to_prune, last_block_can_prune, count);
}

void BlockManager::UpdatePruneLock(const std::string& name, const PruneLockInfo& lock_info) {
    AssertLockHeld(::cs_main);
    m_prune_locks[name] = lock_info;
}

CBlockIndex* BlockManager::InsertBlockIndex(const uint256& hash)
{
    AssertLockHeld(cs_main);

    if (hash.IsNull()) {
        return nullptr;
    }

    const auto [mi, inserted]{m_block_index.try_emplace(hash)};
    CBlockIndex* pindex = &(*mi).second;
    if (inserted) {
        pindex->phashBlock = &((*mi).first);
    }
    return pindex;
}

bool BlockManager::LoadBlockIndex(const std::optional<uint256>& snapshot_blockhash)
{
    if (!m_block_tree_db->LoadBlockIndexGuts(
            GetConsensus(), [this](const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return this->InsertBlockIndex(hash); }, m_interrupt)) {
        return false;
    }

    if (snapshot_blockhash) {
        const std::optional<AssumeutxoData> maybe_au_data = GetParams().AssumeutxoForBlockhash(*snapshot_blockhash);
        if (!maybe_au_data) {
            m_opts.notifications.fatalError(strprintf(_("Assumeutxo data not found for the given blockhash '%s'."), snapshot_blockhash->ToString()));
            return false;
        }
        const AssumeutxoData& au_data = *Assert(maybe_au_data);
        m_snapshot_height = au_data.height;
        CBlockIndex* base{LookupBlockIndex(*snapshot_blockhash)};

        // Since m_chain_tx_count (responsible for estimated progress) isn't persisted
        // to disk, we must bootstrap the value for assumedvalid chainstates
        // from the hardcoded assumeutxo chainparams.
        base->m_chain_tx_count = au_data.m_chain_tx_count;
        LogInfo("[snapshot] set m_chain_tx_count=%d for %s", au_data.m_chain_tx_count, snapshot_blockhash->ToString());
    } else {
        // If this isn't called with a snapshot blockhash, make sure the cached snapshot height
        // is null. This is relevant during snapshot completion, when the blockman may be loaded
        // with a height that then needs to be cleared after the snapshot is fully validated.
        m_snapshot_height.reset();
    }

    Assert(m_snapshot_height.has_value() == snapshot_blockhash.has_value());

    // Calculate nChainWork
    std::vector<CBlockIndex*> vSortedByHeight{GetAllBlockIndices()};
    std::sort(vSortedByHeight.begin(), vSortedByHeight.end(),
              CBlockIndexHeightOnlyComparator());

    CBlockIndex* previous_index{nullptr};
    for (CBlockIndex* pindex : vSortedByHeight) {
        if (m_interrupt) return false;
        if (previous_index && pindex->nHeight > previous_index->nHeight + 1) {
            LogError("%s: block index is non-contiguous, index of height %d missing\n", __func__, previous_index->nHeight + 1);
            return false;
        }
        previous_index = pindex;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + GetBlockProof(*pindex);
        pindex->nTimeMax = (pindex->pprev ? std::max(pindex->pprev->nTimeMax, pindex->nTime) : pindex->nTime);

        // We can link the chain of blocks for which we've received transactions at some point, or
        // blocks that are assumed-valid on the basis of snapshot load (see
        // PopulateAndValidateSnapshot()).
        // Pruned nodes may have deleted the block.
        if (pindex->nTx > 0) {
            if (pindex->pprev) {
                if (m_snapshot_height && pindex->nHeight == *m_snapshot_height &&
                        pindex->GetBlockHash() == *snapshot_blockhash) {
                    // Should have been set above; don't disturb it with code below.
                    Assert(pindex->m_chain_tx_count > 0);
                } else if (pindex->pprev->m_chain_tx_count > 0) {
                    pindex->m_chain_tx_count = pindex->pprev->m_chain_tx_count + pindex->nTx;
                } else {
                    pindex->m_chain_tx_count = 0;
                    m_blocks_unlinked.insert(std::make_pair(pindex->pprev, pindex));
                }
            } else {
                pindex->m_chain_tx_count = pindex->nTx;
            }
        }
        if (!(pindex->nStatus & BLOCK_FAILED_MASK) && pindex->pprev && (pindex->pprev->nStatus & BLOCK_FAILED_MASK)) {
            pindex->nStatus |= BLOCK_FAILED_CHILD;
            m_dirty_blockindex.insert(pindex);
        }
        if (pindex->pprev) {
            pindex->BuildSkip();
        }
    }

    return true;
}

bool BlockManager::WriteBlockIndexDB()
{
    AssertLockHeld(::cs_main);
    std::vector<std::pair<int, const CBlockFileInfo*>> vFiles;
    vFiles.reserve(m_dirty_fileinfo.size());
    for (std::set<int>::iterator it = m_dirty_fileinfo.begin(); it != m_dirty_fileinfo.end();) {
        vFiles.emplace_back(*it, &m_blockfile_info[*it]);
        m_dirty_fileinfo.erase(it++);
    }
    std::vector<const CBlockIndex*> vBlocks;
    vBlocks.reserve(m_dirty_blockindex.size());
    for (std::set<CBlockIndex*>::iterator it = m_dirty_blockindex.begin(); it != m_dirty_blockindex.end();) {
        vBlocks.push_back(*it);
        m_dirty_blockindex.erase(it++);
    }
    int max_blockfile = WITH_LOCK(cs_LastBlockFile, return this->MaxBlockfileNum());
    if (!m_block_tree_db->WriteBatchSync(vFiles, max_blockfile, vBlocks)) {
        return false;
    }
    return true;
}

bool BlockManager::LoadBlockIndexDB(const std::optional<uint256>& snapshot_blockhash)
{
    if (!LoadBlockIndex(snapshot_blockhash)) {
        return false;
    }
    int max_blockfile_num{0};

    // Load block file info
    m_block_tree_db->ReadLastBlockFile(max_blockfile_num);
    m_blockfile_info.resize(max_blockfile_num + 1);
    LogInfo("Loading block index db: last block file = %i", max_blockfile_num);
    for (int nFile = 0; nFile <= max_blockfile_num; nFile++) {
        m_block_tree_db->ReadBlockFileInfo(nFile, m_blockfile_info[nFile]);
    }
    LogInfo("Loading block index db: last block file info: %s", m_blockfile_info[max_blockfile_num].ToString());
    for (int nFile = max_blockfile_num + 1; true; nFile++) {
        CBlockFileInfo info;
        if (m_block_tree_db->ReadBlockFileInfo(nFile, info)) {
            m_blockfile_info.push_back(info);
        } else {
            break;
        }
    }

    // Check presence of blk files
    LogInfo("Checking all blk files are present...");
    std::set<int> setBlkDataFiles;
    for (const auto& [_, block_index] : m_block_index) {
        if (block_index.nStatus & BLOCK_HAVE_DATA) {
            setBlkDataFiles.insert(block_index.nFile);
        }
    }
    for (std::set<int>::iterator it = setBlkDataFiles.begin(); it != setBlkDataFiles.end(); it++) {
        FlatFilePos pos(*it, 0);
        if (OpenBlockFile(pos, /*fReadOnly=*/true).IsNull()) {
            return false;
        }
    }

    {
        // Initialize the blockfile cursors.
        LOCK(cs_LastBlockFile);
        for (size_t i = 0; i < m_blockfile_info.size(); ++i) {
            const auto last_height_in_file = m_blockfile_info[i].nHeightLast;
            m_blockfile_cursors[BlockfileTypeForHeight(last_height_in_file)] = {static_cast<int>(i), 0};
        }
    }

    // Check whether we have ever pruned block & undo files
    m_block_tree_db->ReadFlag("prunedblockfiles", m_have_pruned);
    if (m_have_pruned) {
        LogInfo("Loading block index db: Block files have previously been pruned");
    }

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    m_block_tree_db->ReadReindexing(fReindexing);
    if (fReindexing) m_blockfiles_indexed = false;

    return true;
}

void BlockManager::ScanAndUnlinkAlreadyPrunedFiles()
{
    AssertLockHeld(::cs_main);
    int max_blockfile = WITH_LOCK(cs_LastBlockFile, return this->MaxBlockfileNum());
    if (!m_have_pruned) {
        return;
    }

    std::set<int> block_files_to_prune;
    for (int file_number = 0; file_number < max_blockfile; file_number++) {
        if (m_blockfile_info[file_number].nSize == 0) {
            block_files_to_prune.insert(file_number);
        }
    }

    UnlinkPrunedFiles(block_files_to_prune);
}

bool BlockManager::IsBlockPruned(const CBlockIndex& block) const
{
    AssertLockHeld(::cs_main);
    return m_have_pruned && !(block.nStatus & BLOCK_HAVE_DATA) && (block.nTx > 0);
}

const CBlockIndex* BlockManager::GetFirstBlock(const CBlockIndex& upper_block, uint32_t status_mask, const CBlockIndex* lower_block) const
{
    AssertLockHeld(::cs_main);
    const CBlockIndex* last_block = &upper_block;
    assert((last_block->nStatus & status_mask) == status_mask); // 'upper_block' must satisfy the status mask
    while (last_block->pprev && ((last_block->pprev->nStatus & status_mask) == status_mask)) {
        if (lower_block) {
            // Return if we reached the lower_block
            if (last_block == lower_block) return lower_block;
            // if range was surpassed, means that 'lower_block' is not part of the 'upper_block' chain
            // and so far this is not allowed.
            assert(last_block->nHeight >= lower_block->nHeight);
        }
        last_block = last_block->pprev;
    }
    assert(last_block != nullptr);
    return last_block;
}

bool BlockManager::CheckBlockDataAvailability(const CBlockIndex& upper_block, const CBlockIndex& lower_block)
{
    if (!(upper_block.nStatus & BLOCK_HAVE_DATA)) return false;
    return GetFirstBlock(upper_block, BLOCK_HAVE_DATA, &lower_block) == &lower_block;
}

// If we're using -prune with -reindex, then delete block files that will be ignored by the
// reindex.  Since reindexing works by starting at block file 0 and looping until a blockfile
// is missing, do the same here to delete any later block files after a gap.  Also delete all
// rev files since they'll be rewritten by the reindex anyway.  This ensures that m_blockfile_info
// is in sync with what's actually on disk by the time we start downloading, so that pruning
// works correctly.
void BlockManager::CleanupBlockRevFiles() const
{
    std::map<std::string, fs::path> mapBlockFiles;

    // Glob all blk?????.dat and rev?????.dat files from the blocks directory.
    // Remove the rev files immediately and insert the blk file paths into an
    // ordered map keyed by block file index.
    LogInfo("Removing unusable blk?????.dat and rev?????.dat files for -reindex with -prune");
    for (fs::directory_iterator it(m_opts.blocks_dir); it != fs::directory_iterator(); it++) {
        const std::string path = fs::PathToString(it->path().filename());
        if (fs::is_regular_file(*it) &&
            path.length() == 12 &&
            path.ends_with(".dat"))
        {
            if (path.starts_with("blk")) {
                mapBlockFiles[path.substr(3, 5)] = it->path();
            } else if (path.starts_with("rev")) {
                remove(it->path());
            }
        }
    }

    // Remove all block files that aren't part of a contiguous set starting at
    // zero by walking the ordered map (keys are block file indices) by
    // keeping a separate counter.  Once we hit a gap (or if 0 doesn't exist)
    // start removing block files.
    int nContigCounter = 0;
    for (const std::pair<const std::string, fs::path>& item : mapBlockFiles) {
        if (LocaleIndependentAtoi<int>(item.first) == nContigCounter) {
            nContigCounter++;
            continue;
        }
        remove(item.second);
    }
}

CBlockFileInfo* BlockManager::GetBlockFileInfo(size_t n)
{
    LOCK(cs_LastBlockFile);

    return &m_blockfile_info.at(n);
}

bool BlockManager::ReadBlockUndo(CBlockUndo& blockundo, const CBlockIndex& index) const
{
    const FlatFilePos pos{WITH_LOCK(::cs_main, return index.GetUndoPos())};

    // Open history file to read
    AutoFile file{OpenUndoFile(pos, true)};
    if (file.IsNull()) {
        LogError("OpenUndoFile failed for %s while reading block undo", pos.ToString());
        return false;
    }
    BufferedReader filein{std::move(file)};

    try {
        // Read block
        HashVerifier verifier{filein}; // Use HashVerifier, as reserializing may lose data, c.f. commit d3424243

        verifier << index.pprev->GetBlockHash();
        verifier >> blockundo;

        uint256 hashChecksum;
        filein >> hashChecksum;

        // Verify checksum
        if (hashChecksum != verifier.GetHash()) {
            LogError("Checksum mismatch at %s while reading block undo", pos.ToString());
            return false;
        }
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s at %s while reading block undo", e.what(), pos.ToString());
        return false;
    }

    return true;
}

bool BlockManager::FlushUndoFile(int block_file, bool finalize)
{
    FlatFilePos undo_pos_old(block_file, m_blockfile_info[block_file].nUndoSize);
    if (!m_undo_file_seq.Flush(undo_pos_old, finalize)) {
        m_opts.notifications.flushError(_("Flushing undo file to disk failed. This is likely the result of an I/O error."));
        return false;
    }
    return true;
}

bool BlockManager::FlushBlockFile(int blockfile_num, bool fFinalize, bool finalize_undo)
{
    bool success = true;
    LOCK(cs_LastBlockFile);

    if (m_blockfile_info.size() < 1) {
        // Return if we haven't loaded any blockfiles yet. This happens during
        // chainstate init, when we call ChainstateManager::MaybeRebalanceCaches() (which
        // then calls FlushStateToDisk()), resulting in a call to this function before we
        // have populated `m_blockfile_info` via LoadBlockIndexDB().
        return true;
    }
    assert(static_cast<int>(m_blockfile_info.size()) > blockfile_num);

    FlatFilePos block_pos_old(blockfile_num, m_blockfile_info[blockfile_num].nSize);
    if (!m_block_file_seq.Flush(block_pos_old, fFinalize)) {
        m_opts.notifications.flushError(_("Flushing block file to disk failed. This is likely the result of an I/O error."));
        success = false;
    }
    // we do not always flush the undo file, as the chain tip may be lagging behind the incoming blocks,
    // e.g. during IBD or a sync after a node going offline
    if (!fFinalize || finalize_undo) {
        if (!FlushUndoFile(blockfile_num, finalize_undo)) {
            success = false;
        }
    }
    return success;
}

BlockfileType BlockManager::BlockfileTypeForHeight(int height)
{
    if (!m_snapshot_height) {
        return BlockfileType::NORMAL;
    }
    return (height >= *m_snapshot_height) ? BlockfileType::ASSUMED : BlockfileType::NORMAL;
}

bool BlockManager::FlushChainstateBlockFile(int tip_height)
{
    LOCK(cs_LastBlockFile);
    auto& cursor = m_blockfile_cursors[BlockfileTypeForHeight(tip_height)];
    // If the cursor does not exist, it means an assumeutxo snapshot is loaded,
    // but no blocks past the snapshot height have been written yet, so there
    // is no data associated with the chainstate, and it is safe not to flush.
    if (cursor) {
        return FlushBlockFile(cursor->file_num, /*fFinalize=*/false, /*finalize_undo=*/false);
    }
    // No need to log warnings in this case.
    return true;
}

uint64_t BlockManager::CalculateCurrentUsage()
{
    LOCK(cs_LastBlockFile);

    uint64_t retval = 0;
    for (const CBlockFileInfo& file : m_blockfile_info) {
        retval += file.nSize + file.nUndoSize;
    }
    return retval;
}

void BlockManager::UnlinkPrunedFiles(const std::set<int>& setFilesToPrune) const
{
    std::error_code ec;
    for (std::set<int>::iterator it = setFilesToPrune.begin(); it != setFilesToPrune.end(); ++it) {
        FlatFilePos pos(*it, 0);
        const bool removed_blockfile{fs::remove(m_block_file_seq.FileName(pos), ec)};
        const bool removed_undofile{fs::remove(m_undo_file_seq.FileName(pos), ec)};
        if (removed_blockfile || removed_undofile) {
            LogDebug(BCLog::BLOCKSTORAGE, "Prune: %s deleted blk/rev (%05u)\n", __func__, *it);
        }
    }
}

AutoFile BlockManager::OpenBlockFile(const FlatFilePos& pos, bool fReadOnly) const
{
    return AutoFile{m_block_file_seq.Open(pos, fReadOnly), m_obfuscation};
}

/** Open an undo file (rev?????.dat) */
AutoFile BlockManager::OpenUndoFile(const FlatFilePos& pos, bool fReadOnly) const
{
    return AutoFile{m_undo_file_seq.Open(pos, fReadOnly), m_obfuscation};
}

fs::path BlockManager::GetBlockPosFilename(const FlatFilePos& pos) const
{
    return m_block_file_seq.FileName(pos);
}

FlatFilePos BlockManager::FindNextBlockPos(unsigned int nAddSize, unsigned int nHeight, uint64_t nTime)
{
    LOCK(cs_LastBlockFile);

    const BlockfileType chain_type = BlockfileTypeForHeight(nHeight);

    if (!m_blockfile_cursors[chain_type]) {
        // If a snapshot is loaded during runtime, we may not have initialized this cursor yet.
        assert(chain_type == BlockfileType::ASSUMED);
        const auto new_cursor = BlockfileCursor{this->MaxBlockfileNum() + 1};
        m_blockfile_cursors[chain_type] = new_cursor;
        LogDebug(BCLog::BLOCKSTORAGE, "[%s] initializing blockfile cursor to %s\n", chain_type, new_cursor);
    }
    const int last_blockfile = m_blockfile_cursors[chain_type]->file_num;

    int nFile = last_blockfile;
    if (static_cast<int>(m_blockfile_info.size()) <= nFile) {
        m_blockfile_info.resize(nFile + 1);
    }

    bool finalize_undo = false;
    unsigned int max_blockfile_size{MAX_BLOCKFILE_SIZE};
    // Use smaller blockfiles in test-only -fastprune mode - but avoid
    // the possibility of having a block not fit into the block file.
    if (m_opts.fast_prune) {
        max_blockfile_size = 0x10000; // 64kiB
        if (nAddSize >= max_blockfile_size) {
            // dynamically adjust the blockfile size to be larger than the added size
            max_blockfile_size = nAddSize + 1;
        }
    }
    assert(nAddSize < max_blockfile_size);

    while (m_blockfile_info[nFile].nSize + nAddSize >= max_blockfile_size) {
        // when the undo file is keeping up with the block file, we want to flush it explicitly
        // when it is lagging behind (more blocks arrive than are being connected), we let the
        // undo block write case handle it
        finalize_undo = (static_cast<int>(m_blockfile_info[nFile].nHeightLast) ==
                         Assert(m_blockfile_cursors[chain_type])->undo_height);

        // Try the next unclaimed blockfile number
        nFile = this->MaxBlockfileNum() + 1;
        // Set to increment MaxBlockfileNum() for next iteration
        m_blockfile_cursors[chain_type] = BlockfileCursor{nFile};

        if (static_cast<int>(m_blockfile_info.size()) <= nFile) {
            m_blockfile_info.resize(nFile + 1);
        }
    }
    FlatFilePos pos;
    pos.nFile = nFile;
    pos.nPos = m_blockfile_info[nFile].nSize;

    if (nFile != last_blockfile) {
        LogDebug(BCLog::BLOCKSTORAGE, "Leaving block file %i: %s (onto %i) (height %i)\n",
                 last_blockfile, m_blockfile_info[last_blockfile].ToString(), nFile, nHeight);

        // Do not propagate the return code. The flush concerns a previous block
        // and undo file that has already been written to. If a flush fails
        // here, and we crash, there is no expected additional block data
        // inconsistency arising from the flush failure here. However, the undo
        // data may be inconsistent after a crash if the flush is called during
        // a reindex. A flush error might also leave some of the data files
        // untrimmed.
        if (!FlushBlockFile(last_blockfile, /*fFinalize=*/true, finalize_undo)) {
            LogPrintLevel(BCLog::BLOCKSTORAGE, BCLog::Level::Warning,
                          "Failed to flush previous block file %05i (finalize=1, finalize_undo=%i) before opening new block file %05i\n",
                          last_blockfile, finalize_undo, nFile);
        }
        // No undo data yet in the new file, so reset our undo-height tracking.
        m_blockfile_cursors[chain_type] = BlockfileCursor{nFile};
    }

    m_blockfile_info[nFile].AddBlock(nHeight, nTime);
    m_blockfile_info[nFile].nSize += nAddSize;

    bool out_of_space;
    size_t bytes_allocated = m_block_file_seq.Allocate(pos, nAddSize, out_of_space);
    if (out_of_space) {
        m_opts.notifications.fatalError(_("Disk space is too low!"));
        return {};
    }
    if (bytes_allocated != 0 && IsPruneMode()) {
        m_check_for_pruning = true;
    }

    m_dirty_fileinfo.insert(nFile);
    return pos;
}

void BlockManager::UpdateBlockInfo(const CBlock& block, unsigned int nHeight, const FlatFilePos& pos)
{
    LOCK(cs_LastBlockFile);

    // Update the cursor so it points to the last file.
    const BlockfileType chain_type{BlockfileTypeForHeight(nHeight)};
    auto& cursor{m_blockfile_cursors[chain_type]};
    if (!cursor || cursor->file_num < pos.nFile) {
        m_blockfile_cursors[chain_type] = BlockfileCursor{pos.nFile};
    }

    // Update the file information with the current block.
    const unsigned int added_size = ::GetSerializeSize(TX_WITH_WITNESS(block));
    const int nFile = pos.nFile;
    if (static_cast<int>(m_blockfile_info.size()) <= nFile) {
        m_blockfile_info.resize(nFile + 1);
    }
    m_blockfile_info[nFile].AddBlock(nHeight, block.GetBlockTime());
    m_blockfile_info[nFile].nSize = std::max(pos.nPos + added_size, m_blockfile_info[nFile].nSize);
    m_dirty_fileinfo.insert(nFile);
}

bool BlockManager::FindUndoPos(BlockValidationState& state, int nFile, FlatFilePos& pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    pos.nPos = m_blockfile_info[nFile].nUndoSize;
    m_blockfile_info[nFile].nUndoSize += nAddSize;
    m_dirty_fileinfo.insert(nFile);

    bool out_of_space;
    size_t bytes_allocated = m_undo_file_seq.Allocate(pos, nAddSize, out_of_space);
    if (out_of_space) {
        return FatalError(m_opts.notifications, state, _("Disk space is too low!"));
    }
    if (bytes_allocated != 0 && IsPruneMode()) {
        m_check_for_pruning = true;
    }

    return true;
}

bool BlockManager::WriteBlockUndo(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex& block)
{
    AssertLockHeld(::cs_main);
    const BlockfileType type = BlockfileTypeForHeight(block.nHeight);
    auto& cursor = *Assert(WITH_LOCK(cs_LastBlockFile, return m_blockfile_cursors[type]));

    // Write undo information to disk
    if (block.GetUndoPos().IsNull()) {
        FlatFilePos pos;
        const auto blockundo_size{static_cast<uint32_t>(GetSerializeSize(blockundo))};
        if (!FindUndoPos(state, block.nFile, pos, blockundo_size + UNDO_DATA_DISK_OVERHEAD)) {
            LogError("FindUndoPos failed for %s while writing block undo", pos.ToString());
            return false;
        }

        // Open history file to append
        AutoFile file{OpenUndoFile(pos)};
        if (file.IsNull()) {
            LogError("OpenUndoFile failed for %s while writing block undo", pos.ToString());
            return FatalError(m_opts.notifications, state, _("Failed to write undo data."));
        }
        {
            BufferedWriter fileout{file};

            // Write index header
            fileout << GetParams().MessageStart() << blockundo_size;
            pos.nPos += STORAGE_HEADER_BYTES;
            {
                // Calculate checksum
                HashWriter hasher{};
                hasher << block.pprev->GetBlockHash() << blockundo;
                // Write undo data & checksum
                fileout << blockundo << hasher.GetHash();
            }
            // BufferedWriter will flush pending data to file when fileout goes out of scope.
        }

        // Make sure that the file is closed before we call `FlushUndoFile`.
        if (file.fclose() != 0) {
            LogError("Failed to close block undo file %s: %s", pos.ToString(), SysErrorString(errno));
            return FatalError(m_opts.notifications, state, _("Failed to close block undo file."));
        }

        // rev files are written in block height order, whereas blk files are written as blocks come in (often out of order)
        // we want to flush the rev (undo) file once we've written the last block, which is indicated by the last height
        // in the block file info as below; note that this does not catch the case where the undo writes are keeping up
        // with the block writes (usually when a synced up node is getting newly mined blocks) -- this case is caught in
        // the FindNextBlockPos function
        if (pos.nFile < cursor.file_num && static_cast<uint32_t>(block.nHeight) == m_blockfile_info[pos.nFile].nHeightLast) {
            // Do not propagate the return code, a failed flush here should not
            // be an indication for a failed write. If it were propagated here,
            // the caller would assume the undo data not to be written, when in
            // fact it is. Note though, that a failed flush might leave the data
            // file untrimmed.
            if (!FlushUndoFile(pos.nFile, true)) {
                LogPrintLevel(BCLog::BLOCKSTORAGE, BCLog::Level::Warning, "Failed to flush undo file %05i\n", pos.nFile);
            }
        } else if (pos.nFile == cursor.file_num && block.nHeight > cursor.undo_height) {
            cursor.undo_height = block.nHeight;
        }
        // update nUndoPos in block index
        block.nUndoPos = pos.nPos;
        block.nStatus |= BLOCK_HAVE_UNDO;
        m_dirty_blockindex.insert(&block);
    }

    return true;
}

bool BlockManager::ReadBlock(CBlock& block, const FlatFilePos& pos, const std::optional<uint256>& expected_hash) const
{
    block.SetNull();

    // Open history file to read
    std::vector<std::byte> block_data;
    if (!ReadRawBlock(block_data, pos)) {
        return false;
    }

    try {
        // Read block
        SpanReader{block_data} >> TX_WITH_WITNESS(block);
    } catch (const std::exception& e) {
        LogError("Deserialize or I/O error - %s at %s while reading block", e.what(), pos.ToString());
        return false;
    }

    const auto block_hash{block.GetHash()};

    // Check the header
    if (!CheckProofOfWork(block_hash, block.nBits, GetConsensus())) {
        LogError("Errors in block header at %s while reading block", pos.ToString());
        return false;
    }

    // Signet only: check block solution
    if (GetConsensus().signet_blocks && !CheckSignetBlockSolution(block, GetConsensus())) {
        LogError("Errors in block solution at %s while reading block", pos.ToString());
        return false;
    }

    if (expected_hash && block_hash != *expected_hash) {
        LogError("GetHash() doesn't match index at %s while reading block (%s != %s)",
                 pos.ToString(), block_hash.ToString(), expected_hash->ToString());
        return false;
    }

    return true;
}

bool BlockManager::ReadBlock(CBlock& block, const CBlockIndex& index) const
{
    const FlatFilePos block_pos{WITH_LOCK(cs_main, return index.GetBlockPos())};
    return ReadBlock(block, block_pos, index.GetBlockHash());
}

bool BlockManager::ReadRawBlock(std::vector<std::byte>& block, const FlatFilePos& pos) const
{
    if (pos.nPos < STORAGE_HEADER_BYTES) {
        // If nPos is less than STORAGE_HEADER_BYTES, we can't read the header that precedes the block data
        // This would cause an unsigned integer underflow when trying to position the file cursor
        // This can happen after pruning or default constructed positions
        LogError("Failed for %s while reading raw block storage header", pos.ToString());
        return false;
    }
    AutoFile filein{OpenBlockFile({pos.nFile, pos.nPos - STORAGE_HEADER_BYTES}, /*fReadOnly=*/true)};
    if (filein.IsNull()) {
        LogError("OpenBlockFile failed for %s while reading raw block", pos.ToString());
        return false;
    }

    try {
        MessageStartChars blk_start;
        unsigned int blk_size;

        filein >> blk_start >> blk_size;

        if (blk_start != GetParams().MessageStart()) {
            LogError("Block magic mismatch for %s: %s versus expected %s while reading raw block",
                pos.ToString(), HexStr(blk_start), HexStr(GetParams().MessageStart()));
            return false;
        }

        if (blk_size > MAX_SIZE) {
            LogError("Block data is larger than maximum deserialization size for %s: %s versus %s while reading raw block",
                pos.ToString(), blk_size, MAX_SIZE);
            return false;
        }

        block.resize(blk_size); // Zeroing of memory is intentional here
        filein.read(block);
    } catch (const std::exception& e) {
        LogError("Read from block file failed: %s for %s while reading raw block", e.what(), pos.ToString());
        return false;
    }

    return true;
}

FlatFilePos BlockManager::WriteBlock(const CBlock& block, int nHeight)
{
    const unsigned int block_size{static_cast<unsigned int>(GetSerializeSize(TX_WITH_WITNESS(block)))};
    FlatFilePos pos{FindNextBlockPos(block_size + STORAGE_HEADER_BYTES, nHeight, block.GetBlockTime())};
    if (pos.IsNull()) {
        LogError("FindNextBlockPos failed for %s while writing block", pos.ToString());
        return FlatFilePos();
    }
    AutoFile file{OpenBlockFile(pos, /*fReadOnly=*/false)};
    if (file.IsNull()) {
        LogError("OpenBlockFile failed for %s while writing block", pos.ToString());
        m_opts.notifications.fatalError(_("Failed to write block."));
        return FlatFilePos();
    }
    {
        BufferedWriter fileout{file};

        // Write index header
        fileout << GetParams().MessageStart() << block_size;
        pos.nPos += STORAGE_HEADER_BYTES;
        // Write block
        fileout << TX_WITH_WITNESS(block);
    }

    if (file.fclose() != 0) {
        LogError("Failed to close block file %s: %s", pos.ToString(), SysErrorString(errno));
        m_opts.notifications.fatalError(_("Failed to close file when writing block."));
        return FlatFilePos();
    }

    return pos;
}

static auto InitBlocksdirXorKey(const BlockManager::Options& opts)
{
    // Bytes are serialized without length indicator, so this is also the exact
    // size of the XOR-key file.
    std::array<std::byte, Obfuscation::KEY_SIZE> obfuscation{};

    // Consider this to be the first run if the blocksdir contains only hidden
    // files (those which start with a .). Checking for a fully-empty dir would
    // be too aggressive as a .lock file may have already been written.
    bool first_run = true;
    for (const auto& entry : fs::directory_iterator(opts.blocks_dir)) {
        const std::string path = fs::PathToString(entry.path().filename());
        if (!entry.is_regular_file() || !path.starts_with('.')) {
            first_run = false;
            break;
        }
    }

    if (opts.use_xor && first_run) {
        // Only use random fresh key when the boolean option is set and on the
        // very first start of the program.
        FastRandomContext{}.fillrand(obfuscation);
    }

    const fs::path xor_key_path{opts.blocks_dir / "xor.dat"};
    if (fs::exists(xor_key_path)) {
        // A pre-existing xor key file has priority.
        AutoFile xor_key_file{fsbridge::fopen(xor_key_path, "rb")};
        xor_key_file >> obfuscation;
    } else {
        // Create initial or missing xor key file
        AutoFile xor_key_file{fsbridge::fopen(xor_key_path,
#ifdef __MINGW64__
            "wb" // Temporary workaround for https://github.com/bitcoin/bitcoin/issues/30210
#else
            "wbx"
#endif
        )};
        xor_key_file << obfuscation;
        if (xor_key_file.fclose() != 0) {
            throw std::runtime_error{strprintf("Error closing XOR key file %s: %s",
                                               fs::PathToString(xor_key_path),
                                               SysErrorString(errno))};
        }
    }
    // If the user disabled the key, it must be zero.
    if (!opts.use_xor && obfuscation != decltype(obfuscation){}) {
        throw std::runtime_error{
            strprintf("The blocksdir XOR-key can not be disabled when a random key was already stored! "
                      "Stored key: '%s', stored path: '%s'.",
                      HexStr(obfuscation), fs::PathToString(xor_key_path)),
        };
    }
    LogInfo("Using obfuscation key for blocksdir *.dat files (%s): '%s'\n", fs::PathToString(opts.blocks_dir), HexStr(obfuscation));
    return Obfuscation{obfuscation};
}

BlockManager::BlockManager(const util::SignalInterrupt& interrupt, Options opts)
    : m_prune_mode{opts.prune_target > 0},
      m_obfuscation{InitBlocksdirXorKey(opts)},
      m_opts{std::move(opts)},
      m_block_file_seq{FlatFileSeq{m_opts.blocks_dir, "blk", m_opts.fast_prune ? 0x4000 /* 16kB */ : BLOCKFILE_CHUNK_SIZE}},
      m_undo_file_seq{FlatFileSeq{m_opts.blocks_dir, "rev", UNDOFILE_CHUNK_SIZE}},
      m_interrupt{interrupt}
{
    m_block_tree_db = std::make_unique<BlockTreeDB>(m_opts.block_tree_db_params);

    if (m_opts.block_tree_db_params.wipe_data) {
        m_block_tree_db->WriteReindexing(true);
        m_blockfiles_indexed = false;
        // If we're reindexing in prune mode, wipe away unusable block files and all undo data files
        if (m_prune_mode) {
            CleanupBlockRevFiles();
        }
    }
}

class ImportingNow
{
    std::atomic<bool>& m_importing;

public:
    ImportingNow(std::atomic<bool>& importing) : m_importing{importing}
    {
        assert(m_importing == false);
        m_importing = true;
    }
    ~ImportingNow()
    {
        assert(m_importing == true);
        m_importing = false;
    }
};

void ImportBlocks(ChainstateManager& chainman, std::span<const fs::path> import_paths)
{
    ImportingNow imp{chainman.m_blockman.m_importing};

    // -reindex
    if (!chainman.m_blockman.m_blockfiles_indexed) {
        int nFile = 0;
        // Map of disk positions for blocks with unknown parent (only used for reindex);
        // parent hash -> child disk position, multiple children can have the same parent.
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        while (true) {
            FlatFilePos pos(nFile, 0);
            if (!fs::exists(chainman.m_blockman.GetBlockPosFilename(pos))) {
                break; // No block files left to reindex
            }
            AutoFile file{chainman.m_blockman.OpenBlockFile(pos, /*fReadOnly=*/true)};
            if (file.IsNull()) {
                break; // This error is logged in OpenBlockFile
            }
            LogInfo("Reindexing block file blk%05u.dat...", (unsigned int)nFile);
            chainman.LoadExternalBlockFile(file, &pos, &blocks_with_unknown_parent);
            if (chainman.m_interrupt) {
                LogInfo("Interrupt requested. Exit reindexing.");
                return;
            }
            nFile++;
        }
        WITH_LOCK(::cs_main, chainman.m_blockman.m_block_tree_db->WriteReindexing(false));
        chainman.m_blockman.m_blockfiles_indexed = true;
        LogInfo("Reindexing finished");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        chainman.ActiveChainstate().LoadGenesisBlock();
    }

    // -loadblock=
    for (const fs::path& path : import_paths) {
        AutoFile file{fsbridge::fopen(path, "rb")};
        if (!file.IsNull()) {
            LogInfo("Importing blocks file %s...", fs::PathToString(path));
            chainman.LoadExternalBlockFile(file);
            if (chainman.m_interrupt) {
                LogInfo("Interrupt requested. Exit block importing.");
                return;
            }
        } else {
            LogPrintf("Warning: Could not open blocks file %s\n", fs::PathToString(path));
        }
    }

    // scan for better chains in the block chain database, that are not yet connected in the active best chain

    // We can't hold cs_main during ActivateBestChain even though we're accessing
    // the chainman unique_ptrs since ABC requires us not to be holding cs_main, so retrieve
    // the relevant pointers before the ABC call.
    for (Chainstate* chainstate : WITH_LOCK(::cs_main, return chainman.GetAll())) {
        BlockValidationState state;
        if (!chainstate->ActivateBestChain(state, nullptr)) {
            chainman.GetNotifications().fatalError(strprintf(_("Failed to connect best block (%s)."), state.ToString()));
            return;
        }
    }
    // End scope of ImportingNow
}

std::ostream& operator<<(std::ostream& os, const BlockfileType& type) {
    switch(type) {
        case BlockfileType::NORMAL: os << "normal"; break;
        case BlockfileType::ASSUMED: os << "assumed"; break;
        default: os.setstate(std::ios_base::failbit);
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const BlockfileCursor& cursor) {
    os << strprintf("BlockfileCursor(file_num=%d, undo_height=%d)", cursor.file_num, cursor.undo_height);
    return os;
}
} // namespace node
