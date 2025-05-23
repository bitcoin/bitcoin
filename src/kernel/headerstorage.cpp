// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/headerstorage.h>

#include <chain.h>
#include <logging.h>
#include <pow.h>
#include <streams.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/signalinterrupt.h>

namespace kernel {

static void WriteReindexingImpl(AutoFile& file, bool fReindexing)
{
    file.seek(HEADER_FILE_REINDEX_FLAG_POS, SEEK_SET);
    file << fReindexing;
}

static void WriteLastBlock(AutoFile& file, int32_t last_file)
{
    file.seek(BLOCK_FILES_LAST_BLOCK_POS, SEEK_SET);
    file << last_file;
}

static void WritePrunedImpl(AutoFile& file, bool prune)
{
    file.seek(BLOCK_FILES_PRUNE_FLAG_POS, SEEK_SET);
    file << prune;
}

static int64_t ReadHeaderFileDataEnd(AutoFile& file)
{
    int64_t data_end;
    file.seek(HEADER_FILE_DATA_END_POS, SEEK_SET);
    file >> data_end;
    return data_end;
}

static void WriteHeaderFileDataEnd(AutoFile& file, int64_t end)
{
    file.seek(HEADER_FILE_DATA_END_POS, SEEK_SET);
    file << end;
}

static int32_t CalculateBlockFilesPos(int nFile)
{
    // start position + nFile * (message start chars + serialized size of BlockFileInfoWrapper)
    return BLOCK_FILES_DATA_START_POS + nFile * (4 + 36);
}

void BlockTreeStore::CheckMagicAndVersion() const
{
    {
        auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb")}};
        if (file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        uint32_t magic;
        file >> magic;
        if (magic != HEADER_FILE_MAGIC) {
            throw BlockTreeStoreError("Invalid header file magic");
        }
        uint32_t version;
        file >> version;
        if (version != HEADER_FILE_VERSION) {
            throw BlockTreeStoreError("Invalid header file version");
        }
    }

    {
        auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb")}};
        if (file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        uint32_t magic;
        file >> magic;
        if (magic != BLOCK_FILES_FILE_MAGIC) {
            throw BlockTreeStoreError("Invalid block files file magic");
        }
        uint32_t version;
        file >> version;
        if (version != BLOCK_FILES_FILE_VERSION) {
            throw BlockTreeStoreError("Invalid block files file version");
        }
    }
}

BlockTreeStore::BlockTreeStore(const fs::path& path, const CChainParams& params, bool wipe_data)
    : m_header_file_path{path / HEADER_FILE_NAME},
      m_block_files_file_path{path / BLOCK_FILES_FILE_NAME},
      m_params{params}
{
    fs::create_directories(path);
    if (wipe_data) {
        fs::remove(m_header_file_path);
        fs::remove(m_block_files_file_path);
    }
    bool header_file_exists{fs::exists(m_header_file_path)};
    bool block_files_file_exists{fs::exists(m_block_files_file_path)};
    if (header_file_exists ^ block_files_file_exists) {
        throw BlockTreeStoreError("Block tree store is in an inconsistent state");
    }
    if (!header_file_exists && !block_files_file_exists) {
        CreateHeaderFile();
        CreateBlockFilesFile();
    }
    CheckMagicAndVersion();
}

void BlockTreeStore::CreateHeaderFile() const
{
    {
        FILE* file = fsbridge::fopen(m_header_file_path, "wb");
        if (!file) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        AllocateFileRange(file, 0, m_params.AssumedHeaderStoreSize());
        auto autofile{AutoFile{file}};
        if (!autofile.Commit()) {
            throw BlockTreeStoreError(strprintf("Failed to create header file %s\n", fs::PathToString(m_header_file_path)));
        }
    }

    auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb+")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    file << HEADER_FILE_MAGIC;
    file << HEADER_FILE_VERSION;
    WriteReindexingImpl(file, false);
    WriteHeaderFileDataEnd(file, HEADER_FILE_DATA_START_POS);
    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_header_file_path)));
    }
}

void BlockTreeStore::ReadReindexing(bool& reindexing) const
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    file.seek(HEADER_FILE_REINDEX_FLAG_POS, SEEK_SET);
    file >> reindexing;
}

void BlockTreeStore::WriteReindexing(bool reindexing) const
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb+")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    WriteReindexingImpl(file, reindexing);
    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_header_file_path)));
    }
}

void BlockTreeStore::CreateBlockFilesFile() const
{
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "wb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_block_files_file_path)));
    }
    file << BLOCK_FILES_FILE_MAGIC;
    file << BLOCK_FILES_FILE_VERSION;
    WriteLastBlock(file, 0);
    WritePrunedImpl(file, false);
    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_block_files_file_path)));
    }
}

void BlockTreeStore::ReadLastBlockFile(int32_t& last_block) const
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    file.seek(BLOCK_FILES_LAST_BLOCK_POS, SEEK_SET);
    file >> last_block;
}

void BlockTreeStore::ReadPruned(bool& pruned) const
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    file.seek(BLOCK_FILES_PRUNE_FLAG_POS, SEEK_SET);
    file >> pruned;
}

void BlockTreeStore::WritePruned(bool pruned) const
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb+")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    WritePrunedImpl(file, pruned);
    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_header_file_path)));
    }
}

bool BlockTreeStore::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    LOCK(m_mutex);
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    file.seek(CalculateBlockFilesPos(nFile), SEEK_SET);
    if (file.feof()) {
        // return in case the info was not found
        return false;
    }
    MessageStartChars buf;
    try {
        file >> buf;
    } catch (std::ios_base::failure::exception&) {
        return false;
    }
    if (buf != m_params.MessageStart()) {
        return false;
    }

    BlockFileInfoWrapper info_wrapper;
    file >> info_wrapper;
    info.nBlocks = info_wrapper.nBlocks;
    info.nSize = info_wrapper.nSize;
    info.nUndoSize = info_wrapper.nUndoSize;
    info.nHeightFirst = info_wrapper.nHeightFirst;
    info.nHeightLast = info_wrapper.nHeightLast;
    info.nTimeFirst = info_wrapper.nTimeFirst;
    info.nTimeLast = info_wrapper.nTimeLast;
    return true;
}

bool BlockTreeStore::WriteBatchSync(const std::vector<std::pair<int, CBlockFileInfo*>>& fileInfo, int32_t last_file, const std::vector<CBlockIndex*>& blockinfo)
{
    AssertLockHeld(::cs_main);
    LOCK(m_mutex);

    // Write the block files data
    {
        auto block_files_file{AutoFile{fsbridge::fopen(m_block_files_file_path, "rb+")}};
        if (block_files_file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        block_files_file.seek(BLOCK_FILES_DATA_START_POS, SEEK_SET);
        for (const auto& [file, info] : fileInfo) {
            auto pos{CalculateBlockFilesPos(file)};
            if (block_files_file.tell() != pos) {
                block_files_file.seek(pos, SEEK_SET);
            }
            block_files_file << m_params.MessageStart();
            block_files_file << BlockFileInfoWrapper{info};
        }
        WriteLastBlock(block_files_file, last_file);
        if (!block_files_file.Commit()) {
            throw BlockTreeStoreError(strprintf("Failed to commit block file info batch write to file %s\n", fs::PathToString(m_header_file_path)));
        }
    }

    // Read the header data end position
    int64_t header_data_end;
    {
        auto header_file{AutoFile{fsbridge::fopen(m_header_file_path, "rb")}};
        if (header_file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        header_data_end = ReadHeaderFileDataEnd(header_file);
    }

    // Write the header data
    {
        auto header_file{AutoFile{fsbridge::fopen(m_header_file_path, "rb+")}};
        if (header_file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
        }
        for (CBlockIndex* bi : blockinfo) {
            // Append a new header to the end
            if (bi->header_pos == 0 && header_file.tell() != header_data_end) {
                header_file.seek(header_data_end, SEEK_SET);
            }
            // Seek to the existing header
            if (bi->header_pos != 0 && header_file.tell() != bi->header_pos) {
                header_file.seek(bi->header_pos, SEEK_SET);
            }
            auto disk_bi{CDiskBlockIndex{bi}};
            header_file << m_params.MessageStart();
            header_file << DiskBlockIndexWrapper{&disk_bi};
            // Update the header_pos to the previous end, and data_end to the new end
            if (bi->header_pos == 0) {
                bi->header_pos = header_data_end;
                header_data_end = header_file.tell();
            }
        }
        WriteHeaderFileDataEnd(header_file, header_data_end);
        if (!header_file.Commit()) {
            throw BlockTreeStoreError(strprintf("Failed to commit block index batch write to file %s\n", fs::PathToString(m_header_file_path)));
        }
    }

    return true;
}

bool BlockTreeStore::LoadBlockIndexGuts(
    const Consensus::Params& consensusParams,
    std::function<CBlockIndex*(const uint256&)> insertBlockIndex,
    const util::SignalInterrupt& interrupt)
{
    AssertLockHeld(::cs_main);
    LOCK(m_mutex);

    auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }

    file.seek(HEADER_FILE_DATA_START_POS, SEEK_SET);
    auto data_end_pos{ReadHeaderFileDataEnd(file)};
    while (file.tell() < data_end_pos) {
        if (interrupt) return false;
        DiskBlockIndexWrapper diskindex;
        MessageStartChars buf;
        file >> buf;
        if (buf != m_params.MessageStart()) {
            LogPrintf("did not find message start char");
            break;
        }

        file >> diskindex;
        // Construct block index object
        CBlockIndex* pindexNew = insertBlockIndex(diskindex.ConstructBlockHash());
        pindexNew->pprev = insertBlockIndex(diskindex.hashPrev);
        pindexNew->nHeight = diskindex.nHeight;
        pindexNew->nFile = diskindex.nFile;
        pindexNew->nDataPos = diskindex.nDataPos;
        pindexNew->nUndoPos = diskindex.nUndoPos;
        pindexNew->header_pos = diskindex.header_pos;
        pindexNew->nVersion = diskindex.nVersion;
        pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
        pindexNew->nTime = diskindex.nTime;
        pindexNew->nBits = diskindex.nBits;
        pindexNew->nNonce = diskindex.nNonce;
        pindexNew->nStatus = diskindex.nStatus;
        pindexNew->nTx = diskindex.nTx;

        if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits, consensusParams)) {
            LogError("%s: CheckProofOfWork failed: %s\n", __func__, pindexNew->ToString());
            return false;
        }
    }
    return true;
}

} // namespace kernel
