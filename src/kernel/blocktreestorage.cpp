// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/blocktreestorage.h>

#include <crc32c/include/crc32c/crc32c.h>

#include <chain.h>
#include <logging.h>
#include <pow.h>
#include <streams.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/signalinterrupt.h>

#include <fstream>

namespace kernel {

static uint32_t constexpr BLOCK_FILE_INFO_WRAPPER_SIZE{36};
static uint32_t constexpr DISK_BLOCK_INDEX_WRAPPER_SIZE{112};
static size_t constexpr CHECKSUM_SIZE{sizeof(uint32_t)};
static size_t constexpr FILE_POSITION_SIZE{sizeof(int64_t)};

static int64_t ReadHeaderFileDataEnd(AutoFile& file)
{
    int64_t data_end;
    uint32_t checksum;
    DataStream data;
    file.seek(HEADER_FILE_DATA_END_POS, SEEK_SET);
    file >> data_end;
    data << data_end;
    data << HEADER_FILE_DATA_END_POS;
    uint32_t re_check = crc32c::Crc32c(UCharCast(data.data()), data.size());
    file >> checksum;
    if (re_check != checksum) {
        throw BlockTreeStoreError("Header file data failed integrity check.");
    }
    data >> data_end;
    return data_end;
}

static int64_t CalculateBlockFilesPos(int nFile)
{
    // start position + nFile * (BLOCK_FILE_IFO_WRAPPER_SIZE + checksum)
    return BLOCK_FILES_DATA_START_POS + nFile * (BLOCK_FILE_INFO_WRAPPER_SIZE + CHECKSUM_SIZE);
}

enum ValueType : uint32_t {
    LAST_BLOCK,
    BLOCK_FILE_INFO,
    DISK_BLOCK_INDEX,
    HEADER_DATA_END,
};

const fs::path& BlockTreeStore::GetDataFile(uint32_t value_type) const
{
    switch (value_type) {
    case LAST_BLOCK:
    case BLOCK_FILE_INFO:
        return m_block_files_file_path;
    case DISK_BLOCK_INDEX:
    case HEADER_DATA_END:
        return m_header_file_path;
    }
    throw BlockTreeStoreError("Unrecognized value in block tree store");
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
      m_log_file_path{path / LOG_FILE_NAME},
      m_block_files_file_path{path / BLOCK_FILES_FILE_NAME},
      m_reindex_flag_file_path{path / REINDEX_FLAG_FILE_NAME},
      m_prune_flag_file_path{path / PRUNE_FLAG_FILE_NAME},
      m_params{params}
{
    assert(GetSerializeSize(DiskBlockIndexWrapper{}) == DISK_BLOCK_INDEX_WRAPPER_SIZE);
    assert(GetSerializeSize(BlockFileInfoWrapper{}) == BLOCK_FILE_INFO_WRAPPER_SIZE);
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
    LOCK(m_mutex);
    (void)ApplyLog(); // Ignore an incomplete log file here, the integrity of the data is still intact.
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
        if (autofile.fclose() != 0) {
            throw BlockTreeStoreError(strprintf("Failure when closing created header file %s\n", fs::PathToString(m_header_file_path)));
        }
    }

    auto file{AutoFile{fsbridge::fopen(m_header_file_path, "rb+")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }

    // Write the magic, version, and last block entry (0 on init) with its checksum
    file << HEADER_FILE_MAGIC;
    file << HEADER_FILE_VERSION;
    file.seek(HEADER_FILE_DATA_END_POS, SEEK_SET);
    DataStream data;
    data << HEADER_FILE_DATA_START_POS;
    file << std::span<std::byte>{data};
    data << HEADER_FILE_DATA_END_POS;
    uint32_t checksum = crc32c::Crc32c(UCharCast(data.data()), data.size());
    file << checksum;

    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_header_file_path)));
    }
    if (file.fclose() != 0) {
        throw BlockTreeStoreError(strprintf("Failed to close after write to header file %s\n", fs::PathToString(m_header_file_path)));
    }
}

void BlockTreeStore::ReadReindexing(bool& reindexing) const
{
    LOCK(m_mutex);
    reindexing = fs::exists(m_reindex_flag_file_path);
}

void BlockTreeStore::WriteReindexing(bool reindexing) const
{
    LOCK(m_mutex);
    if (reindexing) {
        std::ofstream{m_reindex_flag_file_path}.close();
    } else {
        fs::remove(m_reindex_flag_file_path);
    }
}

void BlockTreeStore::CreateBlockFilesFile() const
{
    auto file{AutoFile{fsbridge::fopen(m_block_files_file_path, "wb")}};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_block_files_file_path)));
    }

    // Write the magic, version, and last block entry (0 on init) with its checksum
    file << BLOCK_FILES_FILE_MAGIC;
    file << BLOCK_FILES_FILE_VERSION;
    file.seek(BLOCK_FILES_LAST_BLOCK_POS, SEEK_SET);
    DataStream data;
    data << 0;
    file << std::span<std::byte>{data};
    data << BLOCK_FILES_LAST_BLOCK_POS;
    uint32_t checksum = crc32c::Crc32c(UCharCast(data.data()), data.size());
    file << checksum;

    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s\n", fs::PathToString(m_block_files_file_path)));
    }
    if (file.fclose() != 0) {
        throw BlockTreeStoreError(strprintf("Failed to close after write to block files file %s\n", fs::PathToString(m_block_files_file_path)));
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
    DataStream data;
    data << last_block;
    data << BLOCK_FILES_LAST_BLOCK_POS;
    uint32_t re_check = crc32c::Crc32c(UCharCast(data.data()), data.size());
    uint32_t checksum;
    file >> checksum;
    if (re_check != checksum) {
        throw BlockTreeStoreError("Block files data failed integrity check.");
    }
}

void BlockTreeStore::ReadPruned(bool& pruned) const
{
    LOCK(m_mutex);
    pruned = fs::exists(m_prune_flag_file_path);
}

void BlockTreeStore::WritePruned(bool pruned) const
{
    LOCK(m_mutex);
    if (pruned) {
        std::ofstream{m_prune_flag_file_path}.close();
    } else {
        fs::remove(m_prune_flag_file_path);
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

    BlockFileInfoWrapper info_wrapper;

    try {
        DataStream data;
        data.resize(BLOCK_FILE_INFO_WRAPPER_SIZE);
        file.read(std::span<std::byte, BLOCK_FILE_INFO_WRAPPER_SIZE>{data});
        data << CalculateBlockFilesPos(nFile);
        data >> info_wrapper;
        data.Rewind();

        uint32_t checksum;
        file >> checksum;
        uint32_t re_check = crc32c::Crc32c(UCharCast(data.data()), BLOCK_FILE_INFO_WRAPPER_SIZE + FILE_POSITION_SIZE);
        if (re_check != checksum) {
            throw BlockTreeStoreError("Block files data failed integrity check.");
        }
    } catch (std::ios_base::failure::exception&) {
        return false;
    }

    info.nBlocks = info_wrapper.nBlocks;
    info.nSize = info_wrapper.nSize;
    info.nUndoSize = info_wrapper.nUndoSize;
    info.nHeightFirst = info_wrapper.nHeightFirst;
    info.nHeightLast = info_wrapper.nHeightLast;
    info.nTimeFirst = info_wrapper.nTimeFirst;
    info.nTimeLast = info_wrapper.nTimeLast;
    return true;
}

bool BlockTreeStore::ApplyLog() const
{
    AssertLockHeld(m_mutex);

    if (!fs::exists(m_log_file_path)) {
        return false;
    }

    auto log_file{AutoFile{fsbridge::fopen(m_log_file_path, "rb")}};
    if (log_file.IsNull()) {
        return false;
    }

    uint32_t re_rolling_checksum = 0;

    uint32_t number_of_types;
    log_file >> number_of_types;

    // Do a dry run to check the integrity of the log file. This should prevent corrupting the data with a corrupt/incomplete log
    for (uint32_t i = 0; i < number_of_types; i++) {
        uint32_t value_type;
        log_file >> value_type;

        uint32_t type_size;
        log_file >> type_size;
        uint64_t num_iterations;
        log_file >> num_iterations;
        uint32_t entry_size = type_size + FILE_POSITION_SIZE;

        DataStream stream;
        stream.resize(entry_size);

        for (uint32_t j = 0; j < num_iterations; j++) {
            log_file.read(std::span<std::byte>(stream));
            stream.ignore(type_size);
            int64_t pos;
            stream >> pos;

            uint32_t re_checksum = crc32c::Crc32c(UCharCast(stream.data()), entry_size);
            re_rolling_checksum = crc32c::Extend(re_rolling_checksum, UCharCast(stream.data()), entry_size);
            uint32_t checksum;
            log_file >> checksum;
            if (checksum != re_checksum) {
                LogDebug(BCLog::BLOCKSTORAGE, "Found invalid entry in blocktree store log file. Will not apply log.");
                (void)log_file.fclose();
                fs::remove(m_log_file_path);
                return false;
            }

            stream.Rewind();
            stream.resize(entry_size);
        }
    }

    uint32_t rolling_checksum;
    log_file >> rolling_checksum;
    if (rolling_checksum != re_rolling_checksum) {
        LogDebug(BCLog::BLOCKSTORAGE, "Found incomplete blocktree store log file. Will not apply log.");
        (void)log_file.fclose();
        fs::remove(m_log_file_path);
        return false;
    }
    re_rolling_checksum = 0;
    log_file.seek(4, SEEK_SET); // we already read the number of types, so skip ahead of it

    // Run through the file again, but this time write it to the target data file.
    for (uint32_t i = 0; i < number_of_types; i++) {
        uint32_t value_type;
        log_file >> value_type;

        auto data_file_path = GetDataFile(value_type);
        auto data_file{AutoFile{fsbridge::fopen(data_file_path, "rb+")}};
        if (data_file.IsNull()) {
            throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(data_file_path)));
        }

        uint32_t type_size;
        log_file >> type_size;
        uint64_t num_iterations;
        log_file >> num_iterations;
        uint32_t entry_size = type_size + FILE_POSITION_SIZE;

        DataStream stream;
        stream.resize(entry_size);

        for (uint32_t i = 0; i < num_iterations; i++) {
            log_file.read(std::span<std::byte>(stream));
            stream.ignore(type_size);
            int64_t pos;
            stream >> pos;

            uint32_t re_checksum = crc32c::Crc32c(UCharCast(stream.data()), entry_size);
            re_rolling_checksum = crc32c::Extend(re_rolling_checksum, UCharCast(stream.data()), entry_size);
            uint32_t checksum;
            log_file >> checksum;
            if (re_checksum != checksum) {
                throw BlockTreeStoreError("Detected on-disk file corruption. The disk might be nearing its end of life");
            }

            if (data_file.tell() != pos) {
                data_file.seek(pos, SEEK_SET);
            }
            stream.Rewind();

            data_file << std::span<std::byte>{stream.data(), type_size};
            data_file << checksum;
            stream.clear();
            stream.resize(entry_size);

            // TEST ONLY
            if (m_incomplete_log_apply) {
                (void)data_file.fclose();
                return false;
            }
        }

        if (!data_file.Commit()) {
            LogError("Failed to commit write to data file %s", PathToString(data_file_path));
            return false;
        }
        if (data_file.fclose() != 0) {
            LogError("Failed to close after write to data file %s", PathToString(data_file_path));
            return false;
        }
    }

    if (rolling_checksum != re_rolling_checksum) {
        throw BlockTreeStoreError("Detected on-disk file corruption. The disk might be nearing its end of life");
    }

    (void)log_file.fclose();
    fs::remove(m_log_file_path);
    return true;
}

bool BlockTreeStore::WriteBatchSync(const std::vector<std::pair<int, CBlockFileInfo*>>& fileInfo, int32_t last_file, const std::vector<CBlockIndex*>& blockinfo)
{
    AssertLockHeld(::cs_main);
    LOCK(m_mutex);

    // Use a write-ahead log file that gets atomically flushed to the target files.

    { // start log_file scope
    FILE* raw_log_file{fsbridge::fopen(m_log_file_path, "wb")};
    if (!raw_log_file) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s\n", fs::PathToString(m_header_file_path)));
    }
    size_t log_file_prealloc_size{fileInfo.size() * (BLOCK_FILE_INFO_WRAPPER_SIZE + FILE_POSITION_SIZE) + blockinfo.size() * (DISK_BLOCK_INDEX_WRAPPER_SIZE + FILE_POSITION_SIZE)};
    AllocateFileRange(raw_log_file, 0, log_file_prealloc_size);
    auto log_file{AutoFile{raw_log_file}};
    log_file.seek(0, SEEK_SET); // on windows AllocateFileRange moves the pointer to the end, so move it to the beginning again

    DataStream stream;
    stream.reserve(DISK_BLOCK_INDEX_WRAPPER_SIZE + FILE_POSITION_SIZE);
    uint32_t rolling_checksum = 0;

    log_file << uint32_t{4}; // We are writing four different types to the log file for now.

    // Write the last block file number to the log
    log_file << ValueType::LAST_BLOCK;
    log_file << uint32_t{sizeof(uint32_t)};
    log_file << uint64_t{1}; // just the one entry
    stream << last_file;
    stream << BLOCK_FILES_LAST_BLOCK_POS;
    uint32_t checksum = crc32c::Crc32c(UCharCast(stream.data()), stream.size());
    rolling_checksum = crc32c::Extend(rolling_checksum, UCharCast(stream.data()), stream.size());
    log_file << std::span<std::byte>{stream};
    log_file << checksum;
    stream.clear();

    // Write thefileInfo entries to the log
    log_file << ValueType::BLOCK_FILE_INFO;
    log_file << BLOCK_FILE_INFO_WRAPPER_SIZE;
    log_file << uint64_t{fileInfo.size()};
    constexpr size_t block_file_entry_size{BLOCK_FILE_INFO_WRAPPER_SIZE + FILE_POSITION_SIZE};
    for (const auto& [file, info] : fileInfo) {
        int64_t pos{CalculateBlockFilesPos(file)};
        stream << BlockFileInfoWrapper{info};
        stream << pos;
        checksum = crc32c::Crc32c(UCharCast(stream.data()), block_file_entry_size);
        rolling_checksum = crc32c::Extend(rolling_checksum, UCharCast(stream.data()), block_file_entry_size);
        log_file.write(stream);
        log_file << checksum;
        stream.clear();
    }

    // TEST ONLY
    if (m_incomplete_log_write) {
        (void)log_file.fclose();
        return false;
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

    // Write the header data to the log
    log_file << ValueType::DISK_BLOCK_INDEX;
    log_file << DISK_BLOCK_INDEX_WRAPPER_SIZE;
    log_file << uint64_t{blockinfo.size()};
    constexpr size_t header_entry_size{DISK_BLOCK_INDEX_WRAPPER_SIZE + FILE_POSITION_SIZE};

    for (CBlockIndex* bi : blockinfo) {
        int64_t pos = bi->header_pos == 0 ? header_data_end : bi->header_pos;
        auto disk_bi{CDiskBlockIndex{bi}};
        stream << DiskBlockIndexWrapper{&disk_bi};
        stream << pos;
        checksum = crc32c::Crc32c(UCharCast(stream.data()), header_entry_size);
        rolling_checksum = crc32c::Extend(rolling_checksum, UCharCast(stream.data()), header_entry_size);
        log_file.write(stream);
        log_file << checksum;
        stream.clear();
        if (bi->header_pos == 0) {
            bi->header_pos = header_data_end;
            header_data_end += DISK_BLOCK_INDEX_WRAPPER_SIZE + CHECKSUM_SIZE;
        }
    }

    // Write the last header position to the log
    log_file << ValueType::HEADER_DATA_END;
    log_file << uint32_t{FILE_POSITION_SIZE};
    log_file << uint64_t{1}; // just the one entry
    stream << header_data_end;
    stream << HEADER_FILE_DATA_END_POS;
    checksum = crc32c::Crc32c(UCharCast(stream.data()), stream.size());
    rolling_checksum = crc32c::Extend(rolling_checksum, UCharCast(stream.data()), stream.size());
    log_file << std::span<std::byte>{stream.data(), stream.size()};
    log_file << checksum;

    // Finally write the rolling checksum, commit, and close
    log_file << rolling_checksum;
    if (!log_file.Commit()) {
        LogError("Failed to commit write to log file %s", PathToString(m_log_file_path));
        return false;
    }
    if (log_file.fclose() != 0) {
        LogError("Failed to close after write to log file %s", PathToString(m_log_file_path));
        return false;
    }

    } // end log_file scope

    if (!ApplyLog()) {
        LogError("Failed to apply write-ahead log to data files");
        return false;
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

    DataStream pos;
    DiskBlockIndexWrapper diskindex;
    uint32_t checksum;
    uint32_t re_check;
    DataStream data;
    // DataStream clears once all its data is de-serialized. Prevent this from happening by allocating an extra byte.
    // This saves having to resize the DataStream on every iteration.
    data.resize(DISK_BLOCK_INDEX_WRAPPER_SIZE + 1);

    while (file.tell() < data_end_pos) {
        if (interrupt) return false;

        pos << file.tell();
        file.read(std::span<std::byte>{data.data(), DISK_BLOCK_INDEX_WRAPPER_SIZE});
        file >> checksum;
        re_check = crc32c::Crc32c(UCharCast(data.data()), DISK_BLOCK_INDEX_WRAPPER_SIZE);
        re_check = crc32c::Extend(re_check, UCharCast(pos.data()), FILE_POSITION_SIZE);
        if (re_check != checksum) {
            throw BlockTreeStoreError("Header file data failed integrity check");
        }
        data >> diskindex;

        data.Rewind();
        pos.clear();

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
