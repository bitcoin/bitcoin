// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>

#include <chain.h>
#include <clientversion.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <flatfile.h>
#include <hash.h>
#include <kernel/chainparams.h>
#include <kernel/messagestartchars.h>
#include <logging.h>
#include <pow.h>
#include <reverse_iterator.h>
#include <signet.h>
#include <streams.h>
#include <sync.h>
#include <undo.h>
#include <util/batchpriority.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>

#include <map>
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
                    return error("%s: CheckProofOfWork failed: %s", __func__, pindexNew->ToString());
                }

                pcursor->Next();
            } else {
                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}
} // namespace kernel

namespace node {
std::atomic_bool fReindex(false);

bool CBlockIndexWorkComparator::operator()(const CBlockIndex* pa, const CBlockIndex* pb) const
{
    // First sort by most total work, ...
    if (pa->nChainWork > pb->nChainWork) return false;
    if (pa->nChainWork < pb->nChainWork) return true;

    // ... then by earliest time received, ...
    if (pa->nSequenceId < pb->nSequenceId) return false;
    if (pa->nSequenceId > pb->nSequenceId) return true;

    // Use pointer address as tie breaker (should only happen with blocks
    // loaded from disk, as those all have id 0).
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
    pindexNew->nSequenceId = 0;

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

    m_blockfile_info[fileNumber].SetNull();
    m_dirty_fileinfo.insert(fileNumber);
}

void BlockManager::FindFilesToPruneManual(std::set<int>& setFilesToPrune, int nManualPruneHeight, int chain_tip_height)
{
    assert(IsPruneMode() && nManualPruneHeight > 0);

    LOCK2(cs_main, cs_LastBlockFile);
    if (chain_tip_height < 0) {
        return;
    }

    // last block to prune is the lesser of (user-specified height, MIN_BLOCKS_TO_KEEP from the tip)
    unsigned int nLastBlockWeCanPrune = std::min((unsigned)nManualPruneHeight, chain_tip_height - MIN_BLOCKS_TO_KEEP);
    int count = 0;
    for (int fileNumber = 0; fileNumber < m_last_blockfile; fileNumber++) {
        if (m_blockfile_info[fileNumber].nSize == 0 || m_blockfile_info[fileNumber].nHeightLast > nLastBlockWeCanPrune) {
            continue;
        }
        PruneOneBlockFile(fileNumber);
        setFilesToPrune.insert(fileNumber);
        count++;
    }
    LogPrintf("Prune (Manual): prune_height=%d removed %d blk/rev pairs\n", nLastBlockWeCanPrune, count);
}

void BlockManager::FindFilesToPrune(std::set<int>& setFilesToPrune, uint64_t nPruneAfterHeight, int chain_tip_height, int prune_height, bool is_ibd)
{
    LOCK2(cs_main, cs_LastBlockFile);
    if (chain_tip_height < 0 || GetPruneTarget() == 0) {
        return;
    }
    if ((uint64_t)chain_tip_height <= nPruneAfterHeight) {
        return;
    }

    unsigned int nLastBlockWeCanPrune{(unsigned)std::min(prune_height, chain_tip_height - static_cast<int>(MIN_BLOCKS_TO_KEEP))};
    uint64_t nCurrentUsage = CalculateCurrentUsage();
    // We don't check to prune until after we've allocated new space for files
    // So we should leave a buffer under our target to account for another allocation
    // before the next pruning.
    uint64_t nBuffer = BLOCKFILE_CHUNK_SIZE + UNDOFILE_CHUNK_SIZE;
    uint64_t nBytesToPrune;
    int count = 0;

    if (nCurrentUsage + nBuffer >= GetPruneTarget()) {
        // On a prune event, the chainstate DB is flushed.
        // To avoid excessive prune events negating the benefit of high dbcache
        // values, we should not prune too rapidly.
        // So when pruning in IBD, increase the buffer a bit to avoid a re-prune too soon.
        if (is_ibd) {
            // Since this is only relevant during IBD, we use a fixed 10%
            nBuffer += GetPruneTarget() / 10;
        }

        for (int fileNumber = 0; fileNumber < m_last_blockfile; fileNumber++) {
            nBytesToPrune = m_blockfile_info[fileNumber].nSize + m_blockfile_info[fileNumber].nUndoSize;

            if (m_blockfile_info[fileNumber].nSize == 0) {
                continue;
            }

            if (nCurrentUsage + nBuffer < GetPruneTarget()) { // are we below our target?
                break;
            }

            // don't prune files that could have a block within MIN_BLOCKS_TO_KEEP of the main chain's tip but keep scanning
            if (m_blockfile_info[fileNumber].nHeightLast > nLastBlockWeCanPrune) {
                continue;
            }

            PruneOneBlockFile(fileNumber);
            // Queue up the files for removal
            setFilesToPrune.insert(fileNumber);
            nCurrentUsage -= nBytesToPrune;
            count++;
        }
    }

    LogPrint(BCLog::PRUNE, "target=%dMiB actual=%dMiB diff=%dMiB max_prune_height=%d removed %d blk/rev pairs\n",
             GetPruneTarget() / 1024 / 1024, nCurrentUsage / 1024 / 1024,
             (int64_t(GetPruneTarget()) - int64_t(nCurrentUsage)) / 1024 / 1024,
             nLastBlockWeCanPrune, count);
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

bool BlockManager::LoadBlockIndex()
{
    if (!m_block_tree_db->LoadBlockIndexGuts(
            GetConsensus(), [this](const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main) { return this->InsertBlockIndex(hash); }, m_interrupt)) {
        return false;
    }

    // Calculate nChainWork
    std::vector<CBlockIndex*> vSortedByHeight{GetAllBlockIndices()};
    std::sort(vSortedByHeight.begin(), vSortedByHeight.end(),
              CBlockIndexHeightOnlyComparator());

    for (CBlockIndex* pindex : vSortedByHeight) {
        if (m_interrupt) return false;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + GetBlockProof(*pindex);
        pindex->nTimeMax = (pindex->pprev ? std::max(pindex->pprev->nTimeMax, pindex->nTime) : pindex->nTime);

        // We can link the chain of blocks for which we've received transactions at some point, or
        // blocks that are assumed-valid on the basis of snapshot load (see
        // PopulateAndValidateSnapshot()).
        // Pruned nodes may have deleted the block.
        if (pindex->nTx > 0) {
            if (pindex->pprev) {
                if (pindex->pprev->nChainTx > 0) {
                    pindex->nChainTx = pindex->pprev->nChainTx + pindex->nTx;
                } else {
                    pindex->nChainTx = 0;
                    m_blocks_unlinked.insert(std::make_pair(pindex->pprev, pindex));
                }
            } else {
                pindex->nChainTx = pindex->nTx;
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
        vFiles.push_back(std::make_pair(*it, &m_blockfile_info[*it]));
        m_dirty_fileinfo.erase(it++);
    }
    std::vector<const CBlockIndex*> vBlocks;
    vBlocks.reserve(m_dirty_blockindex.size());
    for (std::set<CBlockIndex*>::iterator it = m_dirty_blockindex.begin(); it != m_dirty_blockindex.end();) {
        vBlocks.push_back(*it);
        m_dirty_blockindex.erase(it++);
    }
    if (!m_block_tree_db->WriteBatchSync(vFiles, m_last_blockfile, vBlocks)) {
        return false;
    }
    return true;
}

bool BlockManager::LoadBlockIndexDB()
{
    if (!LoadBlockIndex()) {
        return false;
    }

    // Load block file info
    m_block_tree_db->ReadLastBlockFile(m_last_blockfile);
    m_blockfile_info.resize(m_last_blockfile + 1);
    LogPrintf("%s: last block file = %i\n", __func__, m_last_blockfile);
    for (int nFile = 0; nFile <= m_last_blockfile; nFile++) {
        m_block_tree_db->ReadBlockFileInfo(nFile, m_blockfile_info[nFile]);
    }
    LogPrintf("%s: last block file info: %s\n", __func__, m_blockfile_info[m_last_blockfile].ToString());
    for (int nFile = m_last_blockfile + 1; true; nFile++) {
        CBlockFileInfo info;
        if (m_block_tree_db->ReadBlockFileInfo(nFile, info)) {
            m_blockfile_info.push_back(info);
        } else {
            break;
        }
    }

    // Check presence of blk files
    LogPrintf("Checking all blk files are present...\n");
    std::set<int> setBlkDataFiles;
    for (const auto& [_, block_index] : m_block_index) {
        if (block_index.nStatus & BLOCK_HAVE_DATA) {
            setBlkDataFiles.insert(block_index.nFile);
        }
    }
    for (std::set<int>::iterator it = setBlkDataFiles.begin(); it != setBlkDataFiles.end(); it++) {
        FlatFilePos pos(*it, 0);
        if (AutoFile{OpenBlockFile(pos, true)}.IsNull()) {
            return false;
        }
    }

    // Check whether we have ever pruned block & undo files
    m_block_tree_db->ReadFlag("prunedblockfiles", m_have_pruned);
    if (m_have_pruned) {
        LogPrintf("LoadBlockIndexDB(): Block files have previously been pruned\n");
    }

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    m_block_tree_db->ReadReindexing(fReindexing);
    if (fReindexing) fReindex = true;

    return true;
}

void BlockManager::ScanAndUnlinkAlreadyPrunedFiles()
{
    AssertLockHeld(::cs_main);
    if (!m_have_pruned) {
        return;
    }

    std::set<int> block_files_to_prune;
    for (int file_number = 0; file_number < m_last_blockfile; file_number++) {
        if (m_blockfile_info[file_number].nSize == 0) {
            block_files_to_prune.insert(file_number);
        }
    }

    UnlinkPrunedFiles(block_files_to_prune);
}

const CBlockIndex* BlockManager::GetLastCheckpoint(const CCheckpointData& data)
{
    const MapCheckpoints& checkpoints = data.mapCheckpoints;

    for (const MapCheckpoints::value_type& i : reverse_iterate(checkpoints)) {
        const uint256& hash = i.second;
        const CBlockIndex* pindex = LookupBlockIndex(hash);
        if (pindex) {
            return pindex;
        }
    }
    return nullptr;
}

bool BlockManager::IsBlockPruned(const CBlockIndex* pblockindex)
{
    AssertLockHeld(::cs_main);
    return (m_have_pruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0);
}

const CBlockIndex* BlockManager::GetFirstStoredBlock(const CBlockIndex& upper_block, const CBlockIndex* lower_block)
{
    AssertLockHeld(::cs_main);
    const CBlockIndex* last_block = &upper_block;
    assert(last_block->nStatus & BLOCK_HAVE_DATA); // 'upper_block' must have data
    while (last_block->pprev && (last_block->pprev->nStatus & BLOCK_HAVE_DATA)) {
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
    return GetFirstStoredBlock(upper_block, &lower_block) == &lower_block;
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
    LogPrintf("Removing unusable blk?????.dat and rev?????.dat files for -reindex with -prune\n");
    for (fs::directory_iterator it(m_opts.blocks_dir); it != fs::directory_iterator(); it++) {
        const std::string path = fs::PathToString(it->path().filename());
        if (fs::is_regular_file(*it) &&
            path.length() == 12 &&
            path.substr(8,4) == ".dat")
        {
            if (path.substr(0, 3) == "blk") {
                mapBlockFiles[path.substr(3, 5)] = it->path();
            } else if (path.substr(0, 3) == "rev") {
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

bool BlockManager::UndoWriteToDisk(const CBlockUndo& blockundo, FlatFilePos& pos, const uint256& hashBlock) const
{
    // Open history file to append
    AutoFile fileout{OpenUndoFile(pos)};
    if (fileout.IsNull()) {
        return error("%s: OpenUndoFile failed", __func__);
    }

    // Write index header
    unsigned int nSize = GetSerializeSize(blockundo, CLIENT_VERSION);
    fileout << GetParams().MessageStart() << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0) {
        return error("%s: ftell failed", __func__);
    }
    pos.nPos = (unsigned int)fileOutPos;
    fileout << blockundo;

    // calculate & write checksum
    HashWriter hasher{};
    hasher << hashBlock;
    hasher << blockundo;
    fileout << hasher.GetHash();

    return true;
}

bool BlockManager::UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex& index) const
{
    const FlatFilePos pos{WITH_LOCK(::cs_main, return index.GetUndoPos())};

    if (pos.IsNull()) {
        return error("%s: no undo data available", __func__);
    }

    // Open history file to read
    AutoFile filein{OpenUndoFile(pos, true)};
    if (filein.IsNull()) {
        return error("%s: OpenUndoFile failed", __func__);
    }

    // Read block
    uint256 hashChecksum;
    HashVerifier verifier{filein}; // Use HashVerifier as reserializing may lose data, c.f. commit d342424301013ec47dc146a4beb49d5c9319d80a
    try {
        verifier << index.pprev->GetBlockHash();
        verifier >> blockundo;
        filein >> hashChecksum;
    } catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s", __func__, e.what());
    }

    // Verify checksum
    if (hashChecksum != verifier.GetHash()) {
        return error("%s: Checksum mismatch", __func__);
    }

    return true;
}

void BlockManager::FlushUndoFile(int block_file, bool finalize)
{
    FlatFilePos undo_pos_old(block_file, m_blockfile_info[block_file].nUndoSize);
    if (!UndoFileSeq().Flush(undo_pos_old, finalize)) {
        m_opts.notifications.flushError("Flushing undo file to disk failed. This is likely the result of an I/O error.");
    }
}

void BlockManager::FlushBlockFile(bool fFinalize, bool finalize_undo)
{
    LOCK(cs_LastBlockFile);

    if (m_blockfile_info.size() < 1) {
        // Return if we haven't loaded any blockfiles yet. This happens during
        // chainstate init, when we call ChainstateManager::MaybeRebalanceCaches() (which
        // then calls FlushStateToDisk()), resulting in a call to this function before we
        // have populated `m_blockfile_info` via LoadBlockIndexDB().
        return;
    }
    assert(static_cast<int>(m_blockfile_info.size()) > m_last_blockfile);

    FlatFilePos block_pos_old(m_last_blockfile, m_blockfile_info[m_last_blockfile].nSize);
    if (!BlockFileSeq().Flush(block_pos_old, fFinalize)) {
        m_opts.notifications.flushError("Flushing block file to disk failed. This is likely the result of an I/O error.");
    }
    // we do not always flush the undo file, as the chain tip may be lagging behind the incoming blocks,
    // e.g. during IBD or a sync after a node going offline
    if (!fFinalize || finalize_undo) FlushUndoFile(m_last_blockfile, finalize_undo);
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
        const bool removed_blockfile{fs::remove(BlockFileSeq().FileName(pos), ec)};
        const bool removed_undofile{fs::remove(UndoFileSeq().FileName(pos), ec)};
        if (removed_blockfile || removed_undofile) {
            LogPrint(BCLog::BLOCKSTORAGE, "Prune: %s deleted blk/rev (%05u)\n", __func__, *it);
        }
    }
}

FlatFileSeq BlockManager::BlockFileSeq() const
{
    return FlatFileSeq(m_opts.blocks_dir, "blk", m_opts.fast_prune ? 0x4000 /* 16kb */ : BLOCKFILE_CHUNK_SIZE);
}

FlatFileSeq BlockManager::UndoFileSeq() const
{
    return FlatFileSeq(m_opts.blocks_dir, "rev", UNDOFILE_CHUNK_SIZE);
}

FILE* BlockManager::OpenBlockFile(const FlatFilePos& pos, bool fReadOnly) const
{
    return BlockFileSeq().Open(pos, fReadOnly);
}

/** Open an undo file (rev?????.dat) */
FILE* BlockManager::OpenUndoFile(const FlatFilePos& pos, bool fReadOnly) const
{
    return UndoFileSeq().Open(pos, fReadOnly);
}

fs::path BlockManager::GetBlockPosFilename(const FlatFilePos& pos) const
{
    return BlockFileSeq().FileName(pos);
}

bool BlockManager::FindBlockPos(FlatFilePos& pos, unsigned int nAddSize, unsigned int nHeight, uint64_t nTime, bool fKnown)
{
    LOCK(cs_LastBlockFile);

    unsigned int nFile = fKnown ? pos.nFile : m_last_blockfile;
    if (m_blockfile_info.size() <= nFile) {
        m_blockfile_info.resize(nFile + 1);
    }

    bool finalize_undo = false;
    if (!fKnown) {
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
            finalize_undo = (m_blockfile_info[nFile].nHeightLast == m_undo_height_in_last_blockfile);
            nFile++;
            if (m_blockfile_info.size() <= nFile) {
                m_blockfile_info.resize(nFile + 1);
            }
        }
        pos.nFile = nFile;
        pos.nPos = m_blockfile_info[nFile].nSize;
    }

    if ((int)nFile != m_last_blockfile) {
        if (!fKnown) {
            LogPrint(BCLog::BLOCKSTORAGE, "Leaving block file %i: %s\n", m_last_blockfile, m_blockfile_info[m_last_blockfile].ToString());
        }
        FlushBlockFile(!fKnown, finalize_undo);
        m_last_blockfile = nFile;
        m_undo_height_in_last_blockfile = 0; // No undo data yet in the new file, so reset our undo-height tracking.
    }

    m_blockfile_info[nFile].AddBlock(nHeight, nTime);
    if (fKnown) {
        m_blockfile_info[nFile].nSize = std::max(pos.nPos + nAddSize, m_blockfile_info[nFile].nSize);
    } else {
        m_blockfile_info[nFile].nSize += nAddSize;
    }

    if (!fKnown) {
        bool out_of_space;
        size_t bytes_allocated = BlockFileSeq().Allocate(pos, nAddSize, out_of_space);
        if (out_of_space) {
            m_opts.notifications.fatalError("Disk space is too low!", _("Disk space is too low!"));
            return false;
        }
        if (bytes_allocated != 0 && IsPruneMode()) {
            m_check_for_pruning = true;
        }
    }

    m_dirty_fileinfo.insert(nFile);
    return true;
}

bool BlockManager::FindUndoPos(BlockValidationState& state, int nFile, FlatFilePos& pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    pos.nPos = m_blockfile_info[nFile].nUndoSize;
    m_blockfile_info[nFile].nUndoSize += nAddSize;
    m_dirty_fileinfo.insert(nFile);

    bool out_of_space;
    size_t bytes_allocated = UndoFileSeq().Allocate(pos, nAddSize, out_of_space);
    if (out_of_space) {
        return FatalError(m_opts.notifications, state, "Disk space is too low!", _("Disk space is too low!"));
    }
    if (bytes_allocated != 0 && IsPruneMode()) {
        m_check_for_pruning = true;
    }

    return true;
}

bool BlockManager::WriteBlockToDisk(const CBlock& block, FlatFilePos& pos) const
{
    // Open history file to append
    CAutoFile fileout{OpenBlockFile(pos), CLIENT_VERSION};
    if (fileout.IsNull()) {
        return error("WriteBlockToDisk: OpenBlockFile failed");
    }

    // Write index header
    unsigned int nSize = GetSerializeSize(block, fileout.GetVersion());
    fileout << GetParams().MessageStart() << nSize;

    // Write block
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0) {
        return error("WriteBlockToDisk: ftell failed");
    }
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    return true;
}

bool BlockManager::WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex& block)
{
    AssertLockHeld(::cs_main);
    // Write undo information to disk
    if (block.GetUndoPos().IsNull()) {
        FlatFilePos _pos;
        if (!FindUndoPos(state, block.nFile, _pos, ::GetSerializeSize(blockundo, CLIENT_VERSION) + 40)) {
            return error("ConnectBlock(): FindUndoPos failed");
        }
        if (!UndoWriteToDisk(blockundo, _pos, block.pprev->GetBlockHash())) {
            return FatalError(m_opts.notifications, state, "Failed to write undo data");
        }
        // rev files are written in block height order, whereas blk files are written as blocks come in (often out of order)
        // we want to flush the rev (undo) file once we've written the last block, which is indicated by the last height
        // in the block file info as below; note that this does not catch the case where the undo writes are keeping up
        // with the block writes (usually when a synced up node is getting newly mined blocks) -- this case is caught in
        // the FindBlockPos function
        if (_pos.nFile < m_last_blockfile && static_cast<uint32_t>(block.nHeight) == m_blockfile_info[_pos.nFile].nHeightLast) {
            FlushUndoFile(_pos.nFile, true);
        } else if (_pos.nFile == m_last_blockfile && static_cast<uint32_t>(block.nHeight) > m_undo_height_in_last_blockfile) {
            m_undo_height_in_last_blockfile = block.nHeight;
        }
        // update nUndoPos in block index
        block.nUndoPos = _pos.nPos;
        block.nStatus |= BLOCK_HAVE_UNDO;
        m_dirty_blockindex.insert(&block);
    }

    return true;
}

bool BlockManager::ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos) const
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein{OpenBlockFile(pos, true), CLIENT_VERSION};
    if (filein.IsNull()) {
        return error("ReadBlockFromDisk: OpenBlockFile failed for %s", pos.ToString());
    }

    // Read block
    try {
        filein >> block;
    } catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s at %s", __func__, e.what(), pos.ToString());
    }

    // Check the header
    if (!CheckProofOfWork(block.GetHash(), block.nBits, GetConsensus())) {
        return error("ReadBlockFromDisk: Errors in block header at %s", pos.ToString());
    }

    // Signet only: check block solution
    if (GetConsensus().signet_blocks && !CheckSignetBlockSolution(block, GetConsensus())) {
        return error("ReadBlockFromDisk: Errors in block solution at %s", pos.ToString());
    }

    return true;
}

bool BlockManager::ReadBlockFromDisk(CBlock& block, const CBlockIndex& index) const
{
    const FlatFilePos block_pos{WITH_LOCK(cs_main, return index.GetBlockPos())};

    if (!ReadBlockFromDisk(block, block_pos)) {
        return false;
    }
    if (block.GetHash() != index.GetBlockHash()) {
        return error("ReadBlockFromDisk(CBlock&, CBlockIndex*): GetHash() doesn't match index for %s at %s",
                     index.ToString(), block_pos.ToString());
    }
    return true;
}

bool BlockManager::ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos) const
{
    FlatFilePos hpos = pos;
    hpos.nPos -= 8; // Seek back 8 bytes for meta header
    AutoFile filein{OpenBlockFile(hpos, true)};
    if (filein.IsNull()) {
        return error("%s: OpenBlockFile failed for %s", __func__, pos.ToString());
    }

    try {
        MessageStartChars blk_start;
        unsigned int blk_size;

        filein >> blk_start >> blk_size;

        if (blk_start != GetParams().MessageStart()) {
            return error("%s: Block magic mismatch for %s: %s versus expected %s", __func__, pos.ToString(),
                         HexStr(blk_start),
                         HexStr(GetParams().MessageStart()));
        }

        if (blk_size > MAX_SIZE) {
            return error("%s: Block data is larger than maximum deserialization size for %s: %s versus %s", __func__, pos.ToString(),
                         blk_size, MAX_SIZE);
        }

        block.resize(blk_size); // Zeroing of memory is intentional here
        filein.read(MakeWritableByteSpan(block));
    } catch (const std::exception& e) {
        return error("%s: Read from block file failed: %s for %s", __func__, e.what(), pos.ToString());
    }

    return true;
}

FlatFilePos BlockManager::SaveBlockToDisk(const CBlock& block, int nHeight, const FlatFilePos* dbp)
{
    unsigned int nBlockSize = ::GetSerializeSize(block, CLIENT_VERSION);
    FlatFilePos blockPos;
    const auto position_known {dbp != nullptr};
    if (position_known) {
        blockPos = *dbp;
    } else {
        // when known, blockPos.nPos points at the offset of the block data in the blk file. that already accounts for
        // the serialization header present in the file (the 4 magic message start bytes + the 4 length bytes = 8 bytes = BLOCK_SERIALIZATION_HEADER_SIZE).
        // we add BLOCK_SERIALIZATION_HEADER_SIZE only for new blocks since they will have the serialization header added when written to disk.
        nBlockSize += static_cast<unsigned int>(BLOCK_SERIALIZATION_HEADER_SIZE);
    }
    if (!FindBlockPos(blockPos, nBlockSize, nHeight, block.GetBlockTime(), position_known)) {
        error("%s: FindBlockPos failed", __func__);
        return FlatFilePos();
    }
    if (!position_known) {
        if (!WriteBlockToDisk(block, blockPos)) {
            m_opts.notifications.fatalError("Failed to write block");
            return FlatFilePos();
        }
    }
    return blockPos;
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

void ImportBlocks(ChainstateManager& chainman, std::vector<fs::path> vImportFiles)
{
    ScheduleBatchPriority();

    {
        ImportingNow imp{chainman.m_blockman.m_importing};

        // -reindex
        if (fReindex) {
            int nFile = 0;
            // Map of disk positions for blocks with unknown parent (only used for reindex);
            // parent hash -> child disk position, multiple children can have the same parent.
            std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
            while (true) {
                FlatFilePos pos(nFile, 0);
                if (!fs::exists(chainman.m_blockman.GetBlockPosFilename(pos))) {
                    break; // No block files left to reindex
                }
                FILE* file = chainman.m_blockman.OpenBlockFile(pos, true);
                if (!file) {
                    break; // This error is logged in OpenBlockFile
                }
                LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
                chainman.LoadExternalBlockFile(file, &pos, &blocks_with_unknown_parent);
                if (chainman.m_interrupt) {
                    LogPrintf("Interrupt requested. Exit %s\n", __func__);
                    return;
                }
                nFile++;
            }
            WITH_LOCK(::cs_main, chainman.m_blockman.m_block_tree_db->WriteReindexing(false));
            fReindex = false;
            LogPrintf("Reindexing finished\n");
            // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
            chainman.ActiveChainstate().LoadGenesisBlock();
        }

        // -loadblock=
        for (const fs::path& path : vImportFiles) {
            FILE* file = fsbridge::fopen(path, "rb");
            if (file) {
                LogPrintf("Importing blocks file %s...\n", fs::PathToString(path));
                chainman.LoadExternalBlockFile(file);
                if (chainman.m_interrupt) {
                    LogPrintf("Interrupt requested. Exit %s\n", __func__);
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
                chainman.GetNotifications().fatalError(strprintf("Failed to connect best block (%s)", state.ToString()));
                return;
            }
        }
    } // End scope of ImportingNow
}
} // namespace node
