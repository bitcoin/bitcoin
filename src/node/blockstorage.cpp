// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>

#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <consensus/validation.h>
#include <flatfile.h>
#include <fs.h>
#include <hash.h>
#include <pow.h>
#include <shutdown.h>
#include <signet.h>
#include <streams.h>
#include <undo.h>
#include <util/syscall_sandbox.h>
#include <util/system.h>
#include <validation.h>

std::atomic_bool fImporting(false);
std::atomic_bool fReindex(false);
bool fHavePruned = false;
bool fPruneMode = false;
uint64_t nPruneTarget = 0;

// TODO make namespace {
RecursiveMutex cs_LastBlockFile;
std::vector<CBlockFileInfo> vinfoBlockFile;
int nLastBlockFile = 0;
/** Global flag to indicate we should check to see if there are
*  block/undo files that should be deleted.  Set on startup
*  or if we allocate more file space when we're in prune mode
*/
bool fCheckForPruning = false;

/** Dirty block index entries. */
std::set<CBlockIndex*> setDirtyBlockIndex;

/** Dirty block file entries. */
std::set<int> setDirtyFileInfo;
// } // namespace

static FILE* OpenUndoFile(const FlatFilePos& pos, bool fReadOnly = false);
static FlatFileSeq BlockFileSeq();
static FlatFileSeq UndoFileSeq();

bool IsBlockPruned(const CBlockIndex* pblockindex)
{
    return (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0);
}

// If we're using -prune with -reindex, then delete block files that will be ignored by the
// reindex.  Since reindexing works by starting at block file 0 and looping until a blockfile
// is missing, do the same here to delete any later block files after a gap.  Also delete all
// rev files since they'll be rewritten by the reindex anyway.  This ensures that vinfoBlockFile
// is in sync with what's actually on disk by the time we start downloading, so that pruning
// works correctly.
void CleanupBlockRevFiles()
{
    std::map<std::string, fs::path> mapBlockFiles;

    // Glob all blk?????.dat and rev?????.dat files from the blocks directory.
    // Remove the rev files immediately and insert the blk file paths into an
    // ordered map keyed by block file index.
    LogPrintf("Removing unusable blk?????.dat and rev?????.dat files for -reindex with -prune\n");
    fs::path blocksdir = gArgs.GetBlocksDirPath();
    for (fs::directory_iterator it(blocksdir); it != fs::directory_iterator(); it++) {
        if (fs::is_regular_file(*it) &&
            it->path().filename().string().length() == 12 &&
            it->path().filename().string().substr(8,4) == ".dat")
        {
            if (it->path().filename().string().substr(0, 3) == "blk") {
                mapBlockFiles[it->path().filename().string().substr(3, 5)] = it->path();
            } else if (it->path().filename().string().substr(0, 3) == "rev") {
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

std::string CBlockFileInfo::ToString() const
{
    return strprintf("CBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks, nSize, nHeightFirst, nHeightLast, FormatISO8601Date(nTimeFirst), FormatISO8601Date(nTimeLast));
}

CBlockFileInfo* GetBlockFileInfo(size_t n)
{
    LOCK(cs_LastBlockFile);

    return &vinfoBlockFile.at(n);
}

static bool UndoWriteToDisk(const CBlockUndo& blockundo, FlatFilePos& pos, const uint256& hashBlock, const CMessageHeader::MessageStartChars& messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull()) {
        return error("%s: OpenUndoFile failed", __func__);
    }

    // Write index header
    unsigned int nSize = GetSerializeSize(blockundo, fileout.GetVersion());
    fileout << messageStart << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0) {
        return error("%s: ftell failed", __func__);
    }
    pos.nPos = (unsigned int)fileOutPos;
    fileout << blockundo;

    // calculate & write checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << hashBlock;
    hasher << blockundo;
    fileout << hasher.GetHash();

    return true;
}

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex)
{
    FlatFilePos pos = pindex->GetUndoPos();
    if (pos.IsNull()) {
        return error("%s: no undo data available", __func__);
    }

    // Open history file to read
    CAutoFile filein(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        return error("%s: OpenUndoFile failed", __func__);
    }

    // Read block
    uint256 hashChecksum;
    CHashVerifier<CAutoFile> verifier(&filein); // We need a CHashVerifier as reserializing may lose data
    try {
        verifier << pindex->pprev->GetBlockHash();
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

static void FlushUndoFile(int block_file, bool finalize = false)
{
    FlatFilePos undo_pos_old(block_file, vinfoBlockFile[block_file].nUndoSize);
    if (!UndoFileSeq().Flush(undo_pos_old, finalize)) {
        AbortNode("Flushing undo file to disk failed. This is likely the result of an I/O error.");
    }
}

void FlushBlockFile(bool fFinalize = false, bool finalize_undo = false)
{
    LOCK(cs_LastBlockFile);
    FlatFilePos block_pos_old(nLastBlockFile, vinfoBlockFile[nLastBlockFile].nSize);
    if (!BlockFileSeq().Flush(block_pos_old, fFinalize)) {
        AbortNode("Flushing block file to disk failed. This is likely the result of an I/O error.");
    }
    // we do not always flush the undo file, as the chain tip may be lagging behind the incoming blocks,
    // e.g. during IBD or a sync after a node going offline
    if (!fFinalize || finalize_undo) FlushUndoFile(nLastBlockFile, finalize_undo);
}

uint64_t CalculateCurrentUsage()
{
    LOCK(cs_LastBlockFile);

    uint64_t retval = 0;
    for (const CBlockFileInfo& file : vinfoBlockFile) {
        retval += file.nSize + file.nUndoSize;
    }
    return retval;
}

void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune)
{
    for (std::set<int>::iterator it = setFilesToPrune.begin(); it != setFilesToPrune.end(); ++it) {
        FlatFilePos pos(*it, 0);
        fs::remove(BlockFileSeq().FileName(pos));
        fs::remove(UndoFileSeq().FileName(pos));
        LogPrint(BCLog::BLOCKSTORE, "Prune: %s deleted blk/rev (%05u)\n", __func__, *it);
    }
}

static FlatFileSeq BlockFileSeq()
{
    return FlatFileSeq(gArgs.GetBlocksDirPath(), "blk", gArgs.GetBoolArg("-fastprune", false) ? 0x4000 /* 16kb */ : BLOCKFILE_CHUNK_SIZE);
}

static FlatFileSeq UndoFileSeq()
{
    return FlatFileSeq(gArgs.GetBlocksDirPath(), "rev", UNDOFILE_CHUNK_SIZE);
}

FILE* OpenBlockFile(const FlatFilePos& pos, bool fReadOnly)
{
    return BlockFileSeq().Open(pos, fReadOnly);
}

/** Open an undo file (rev?????.dat) */
static FILE* OpenUndoFile(const FlatFilePos& pos, bool fReadOnly)
{
    return UndoFileSeq().Open(pos, fReadOnly);
}

fs::path GetBlockPosFilename(const FlatFilePos& pos)
{
    return BlockFileSeq().FileName(pos);
}

bool FindBlockPos(FlatFilePos& pos, unsigned int nAddSize, unsigned int nHeight, CChain& active_chain, uint64_t nTime, bool fKnown = false)
{
    LOCK(cs_LastBlockFile);

    unsigned int nFile = fKnown ? pos.nFile : nLastBlockFile;
    if (vinfoBlockFile.size() <= nFile) {
        vinfoBlockFile.resize(nFile + 1);
    }

    bool finalize_undo = false;
    if (!fKnown) {
        while (vinfoBlockFile[nFile].nSize + nAddSize >= (gArgs.GetBoolArg("-fastprune", false) ? 0x10000 /* 64kb */ : MAX_BLOCKFILE_SIZE)) {
            // when the undo file is keeping up with the block file, we want to flush it explicitly
            // when it is lagging behind (more blocks arrive than are being connected), we let the
            // undo block write case handle it
            finalize_undo = (vinfoBlockFile[nFile].nHeightLast == (unsigned int)active_chain.Tip()->nHeight);
            nFile++;
            if (vinfoBlockFile.size() <= nFile) {
                vinfoBlockFile.resize(nFile + 1);
            }
        }
        pos.nFile = nFile;
        pos.nPos = vinfoBlockFile[nFile].nSize;
    }

    if ((int)nFile != nLastBlockFile) {
        if (!fKnown) {
            LogPrint(BCLog::BLOCKSTORE, "Leaving block file %i: %s\n", nLastBlockFile, vinfoBlockFile[nLastBlockFile].ToString());
        }
        FlushBlockFile(!fKnown, finalize_undo);
        nLastBlockFile = nFile;
    }

    vinfoBlockFile[nFile].AddBlock(nHeight, nTime);
    if (fKnown) {
        vinfoBlockFile[nFile].nSize = std::max(pos.nPos + nAddSize, vinfoBlockFile[nFile].nSize);
    } else {
        vinfoBlockFile[nFile].nSize += nAddSize;
    }

    if (!fKnown) {
        bool out_of_space;
        size_t bytes_allocated = BlockFileSeq().Allocate(pos, nAddSize, out_of_space);
        if (out_of_space) {
            return AbortNode("Disk space is too low!", _("Disk space is too low!"));
        }
        if (bytes_allocated != 0 && fPruneMode) {
            fCheckForPruning = true;
        }
    }

    setDirtyFileInfo.insert(nFile);
    return true;
}

static bool FindUndoPos(BlockValidationState& state, int nFile, FlatFilePos& pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    pos.nPos = vinfoBlockFile[nFile].nUndoSize;
    vinfoBlockFile[nFile].nUndoSize += nAddSize;
    setDirtyFileInfo.insert(nFile);

    bool out_of_space;
    size_t bytes_allocated = UndoFileSeq().Allocate(pos, nAddSize, out_of_space);
    if (out_of_space) {
        return AbortNode(state, "Disk space is too low!", _("Disk space is too low!"));
    }
    if (bytes_allocated != 0 && fPruneMode) {
        fCheckForPruning = true;
    }

    return true;
}

static bool WriteBlockToDisk(const CBlock& block, FlatFilePos& pos, const CMessageHeader::MessageStartChars& messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull()) {
        return error("WriteBlockToDisk: OpenBlockFile failed");
    }

    // Write index header
    unsigned int nSize = GetSerializeSize(block, fileout.GetVersion());
    fileout << messageStart << nSize;

    // Write block
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0) {
        return error("WriteBlockToDisk: ftell failed");
    }
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    return true;
}

bool WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex* pindex, const CChainParams& chainparams)
{
    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull()) {
        FlatFilePos _pos;
        if (!FindUndoPos(state, pindex->nFile, _pos, ::GetSerializeSize(blockundo, CLIENT_VERSION) + 40)) {
            return error("ConnectBlock(): FindUndoPos failed");
        }
        if (!UndoWriteToDisk(blockundo, _pos, pindex->pprev->GetBlockHash(), chainparams.MessageStart())) {
            return AbortNode(state, "Failed to write undo data");
        }
        // rev files are written in block height order, whereas blk files are written as blocks come in (often out of order)
        // we want to flush the rev (undo) file once we've written the last block, which is indicated by the last height
        // in the block file info as below; note that this does not catch the case where the undo writes are keeping up
        // with the block writes (usually when a synced up node is getting newly mined blocks) -- this case is caught in
        // the FindBlockPos function
        if (_pos.nFile < nLastBlockFile && static_cast<uint32_t>(pindex->nHeight) == vinfoBlockFile[_pos.nFile].nHeightLast) {
            FlushUndoFile(_pos.nFile, true);
        }

        // update nUndoPos in block index
        pindex->nUndoPos = _pos.nPos;
        pindex->nStatus |= BLOCK_HAVE_UNDO;
        setDirtyBlockIndex.insert(pindex);
    }

    return true;
}

bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams)
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
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
    if (!CheckProofOfWork(block.GetHash(), block.nBits, consensusParams)) {
        return error("ReadBlockFromDisk: Errors in block header at %s", pos.ToString());
    }

    // Signet only: check block solution
    if (consensusParams.signet_blocks && !CheckSignetBlockSolution(block, consensusParams)) {
        return error("ReadBlockFromDisk: Errors in block solution at %s", pos.ToString());
    }

    return true;
}

bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    const FlatFilePos block_pos{WITH_LOCK(cs_main, return pindex->GetBlockPos())};

    if (!ReadBlockFromDisk(block, block_pos, consensusParams)) {
        return false;
    }
    if (block.GetHash() != pindex->GetBlockHash()) {
        return error("ReadBlockFromDisk(CBlock&, CBlockIndex*): GetHash() doesn't match index for %s at %s",
                     pindex->ToString(), block_pos.ToString());
    }
    return true;
}

bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start)
{
    FlatFilePos hpos = pos;
    hpos.nPos -= 8; // Seek back 8 bytes for meta header
    CAutoFile filein(OpenBlockFile(hpos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        return error("%s: OpenBlockFile failed for %s", __func__, pos.ToString());
    }

    try {
        CMessageHeader::MessageStartChars blk_start;
        unsigned int blk_size;

        filein >> blk_start >> blk_size;

        if (memcmp(blk_start, message_start, CMessageHeader::MESSAGE_START_SIZE)) {
            return error("%s: Block magic mismatch for %s: %s versus expected %s", __func__, pos.ToString(),
                         HexStr(blk_start),
                         HexStr(message_start));
        }

        if (blk_size > MAX_SIZE) {
            return error("%s: Block data is larger than maximum deserialization size for %s: %s versus %s", __func__, pos.ToString(),
                         blk_size, MAX_SIZE);
        }

        block.resize(blk_size); // Zeroing of memory is intentional here
        filein.read((char*)block.data(), blk_size);
    } catch (const std::exception& e) {
        return error("%s: Read from block file failed: %s for %s", __func__, e.what(), pos.ToString());
    }

    return true;
}

bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start)
{
    FlatFilePos block_pos;
    {
        LOCK(cs_main);
        block_pos = pindex->GetBlockPos();
    }

    return ReadRawBlockFromDisk(block, block_pos, message_start);
}

/** Store block on disk. If dbp is non-nullptr, the file is known to already reside on disk */
FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp)
{
    unsigned int nBlockSize = ::GetSerializeSize(block, CLIENT_VERSION);
    FlatFilePos blockPos;
    if (dbp != nullptr) {
        blockPos = *dbp;
    }
    if (!FindBlockPos(blockPos, nBlockSize + 8, nHeight, active_chain, block.GetBlockTime(), dbp != nullptr)) {
        error("%s: FindBlockPos failed", __func__);
        return FlatFilePos();
    }
    if (dbp == nullptr) {
        if (!WriteBlockToDisk(block, blockPos, chainparams.MessageStart())) {
            AbortNode("Failed to write block");
            return FlatFilePos();
        }
    }
    return blockPos;
}

struct CImportingNow {
    CImportingNow()
    {
        assert(fImporting == false);
        fImporting = true;
    }

    ~CImportingNow()
    {
        assert(fImporting == true);
        fImporting = false;
    }
};

void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args)
{
    SetSyscallSandboxPolicy(SyscallSandboxPolicy::INITIALIZATION_LOAD_BLOCKS);
    ScheduleBatchPriority();

    {
        CImportingNow imp;

        // -reindex
        if (fReindex) {
            int nFile = 0;
            while (true) {
                FlatFilePos pos(nFile, 0);
                if (!fs::exists(GetBlockPosFilename(pos))) {
                    break; // No block files left to reindex
                }
                FILE* file = OpenBlockFile(pos, true);
                if (!file) {
                    break; // This error is logged in OpenBlockFile
                }
                LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
                chainman.ActiveChainstate().LoadExternalBlockFile(file, &pos);
                if (ShutdownRequested()) {
                    LogPrintf("Shutdown requested. Exit %s\n", __func__);
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
                LogPrintf("Importing blocks file %s...\n", path.string());
                chainman.ActiveChainstate().LoadExternalBlockFile(file);
                if (ShutdownRequested()) {
                    LogPrintf("Shutdown requested. Exit %s\n", __func__);
                    return;
                }
            } else {
                LogPrintf("Warning: Could not open blocks file %s\n", path.string());
            }
        }

        // scan for better chains in the block chain database, that are not yet connected in the active best chain

        // We can't hold cs_main during ActivateBestChain even though we're accessing
        // the chainman unique_ptrs since ABC requires us not to be holding cs_main, so retrieve
        // the relevant pointers before the ABC call.
        for (CChainState* chainstate : WITH_LOCK(::cs_main, return chainman.GetAll())) {
            BlockValidationState state;
            if (!chainstate->ActivateBestChain(state, nullptr)) {
                LogPrintf("Failed to connect best block (%s)\n", state.ToString());
                StartShutdown();
                return;
            }
        }

        if (args.GetBoolArg("-stopafterblockimport", DEFAULT_STOPAFTERBLOCKIMPORT)) {
            LogPrintf("Stopping after block import\n");
            StartShutdown();
            return;
        }
    } // End scope of CImportingNow
    chainman.ActiveChainstate().LoadMempool(args);
}
