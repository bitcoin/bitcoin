// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/blocktreestorage.h>

#include <chain.h>
#include <crc32c/include/crc32c/crc32c.h>
#include <kernel/cs_main.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <pow.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/signalinterrupt.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <ios>
#include <span>
#include <system_error>
#include <type_traits>
#include <utility>

namespace kernel {

using Checksum = uint32_t;
using FilePosition = int64_t;

/** A wrapper for creating a constant-sized serialization without varint encoding */
struct BlockFileInfoWrapper : CBlockFileInfo {
    static constexpr size_t SERIALIZED_SIZE{36};

    BlockFileInfoWrapper() = default;

    explicit BlockFileInfoWrapper(const CBlockFileInfo* info) : CBlockFileInfo(*info)
    {
    }

    SERIALIZE_METHODS(BlockFileInfoWrapper, obj)
    {
        READWRITE(obj.nBlocks);
        READWRITE(obj.nSize);
        READWRITE(obj.nUndoSize);
        READWRITE(obj.nHeightFirst);
        READWRITE(obj.nHeightLast);
        READWRITE(obj.nTimeFirst);
        READWRITE(obj.nTimeLast);
    }
};

static FilePosition CalculateBlockFileInfoPosition(int file_index)
{
    assert(file_index >= 0);
    return BLOCK_FILES_FILE_DATA_START_POSITION + file_index * (BlockFileInfoWrapper::SERIALIZED_SIZE + sizeof(Checksum));
}

const fs::path& BlockTreeStore::GetDataFilePath(ValueType value_type) const
{
    switch (value_type) {
    case ValueType::BLOCK_FILE_INFO:
        return m_block_files_file_path;
    case ValueType::DISK_BLOCK_INDEX:
        return m_header_file_path;
    }
    assert(false);
}

static uint8_t ValueSize(const ValueType value_type)
{
    switch (value_type) {
    case ValueType::BLOCK_FILE_INFO:
        return BlockFileInfoWrapper::SERIALIZED_SIZE;
    case ValueType::DISK_BLOCK_INDEX:
        return DiskBlockIndexWrapper::SERIALIZED_SIZE;
    }
    assert(false);
}

static ValueType ReadValueType(AutoFile& file)
{
    std::underlying_type_t<ValueType> raw;
    file >> raw;
    switch (auto value_type{static_cast<ValueType>(raw)}) {
    case kernel::ValueType::BLOCK_FILE_INFO:
    case kernel::ValueType::DISK_BLOCK_INDEX:
        return value_type;
    }
    throw BlockTreeStoreError(strprintf("Unrecognized value type (%u) in block tree store", raw));
}

static void WriteMagicAndVersion(AutoFile& file, uint32_t magic, uint32_t version)
{
    file << magic;
    file << version;
}

static AutoFile OpenFile(const fs::path& path, const std::string& mode)
{
    AutoFile file{fsbridge::fopen(path, mode.c_str())};
    if (file.IsNull()) {
        throw BlockTreeStoreError(strprintf("Unable to open file %s", fs::PathToString(path)));
    }
    return AutoFile{file.release()};
}

static void CreateDataFile(const fs::path& path, uint32_t magic, uint32_t version)
{
    auto file{OpenFile(path, "wb")};

    WriteMagicAndVersion(file, magic, version);

    if (!file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to write file %s", fs::PathToString(path)));
    }
    if (file.fclose() != 0) {
        throw BlockTreeStoreError(strprintf("Failed to close after write to file %s", fs::PathToString(path)));
    }
}

static AutoFile OpenFileAndVerifyHeader(const fs::path& path, uint32_t magic_expected, uint32_t version_expected)
{
    auto file{OpenFile(path, "rb")};
    if (auto magic{ser_readdata32(file)}; magic != magic_expected) {
        throw BlockTreeStoreError(strprintf("Invalid magic in %s: 0x%08x (expected: 0x%08x)", fs::PathToString(path), magic, magic_expected));
    }
    if (auto version{ser_readdata32(file)}; version != version_expected) {
        throw BlockTreeStoreError(strprintf("Invalid version in %s: 0x%08x (expected: 0x%08x)", fs::PathToString(path), version, version_expected));
    }
    return AutoFile{file.release()};
}

BlockTreeStore::BlockTreeStore(const fs::path& path, const OpenMode open_mode)
    : m_header_file_path{path / HEADER_FILE_NAME},
      m_log_file_path{path / LOG_FILE_NAME},
      m_log_flag_file_path{path / LOG_FLAG_FILE_NAME},
      m_block_files_file_path{path / BLOCK_FILES_FILE_NAME},
      m_reindex_flag_file_path{path / REINDEX_FLAG_FILE_NAME},
      m_prune_flag_file_path{path / PRUNE_FLAG_FILE_NAME}
{
    assert(GetSerializeSize(DiskBlockIndexWrapper{}) == DiskBlockIndexWrapper::SERIALIZED_SIZE);
    assert(GetSerializeSize(BlockFileInfoWrapper{}) == BlockFileInfoWrapper::SERIALIZED_SIZE);

    LOCK(m_mutex);
    fs::create_directories(path);

    if (open_mode == OpenMode::WIPE) {
        fs::remove(m_header_file_path);
        fs::remove(m_block_files_file_path);
        fs::remove(m_log_file_path);
        fs::remove(m_log_flag_file_path);
        fs::remove(m_reindex_flag_file_path);
        fs::remove(m_prune_flag_file_path);
    }
    bool header_file_exists{fs::exists(m_header_file_path)};
    bool block_files_file_exists{fs::exists(m_block_files_file_path)};
    if (header_file_exists != block_files_file_exists) {
        throw BlockTreeStoreError("Block tree store is in an inconsistent state");
    }
    if (!header_file_exists && !block_files_file_exists) {
        CreateDataFile(m_header_file_path, HEADER_FILE_MAGIC, HEADER_FILE_VERSION);
        CreateDataFile(m_block_files_file_path, BLOCK_FILES_FILE_MAGIC, BLOCK_FILES_FILE_VERSION);
    }
    (void)OpenFileAndVerifyHeader(m_header_file_path, HEADER_FILE_MAGIC, HEADER_FILE_VERSION);
    (void)OpenFileAndVerifyHeader(m_block_files_file_path, BLOCK_FILES_FILE_MAGIC, BLOCK_FILES_FILE_VERSION);
    if (ApplyLog()) {
        LogInfo("Applied block tree store write-ahead log left over from a previous failure, potentially caused by unclean shutdown or intermittent hardware issue.");
    }
}

void BlockTreeStore::WriteFlag(const fs::path& path, bool value, bool directory_commit) const
{
    if (value) {
        if (auto file{AutoFile{fsbridge::fopen(path, "w")}}; file.IsNull() || file.fclose()) {
            throw BlockTreeStoreError(strprintf("Could not create flag file %s", fs::PathToString(path)));
        }
    } else {
        std::error_code ec;
        fs::remove(path, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) {
            throw BlockTreeStoreError(strprintf("Could not remove flag file %s", fs::PathToString(path)));
        }
    }
    if (directory_commit) {
        DirectoryCommit(path.parent_path());
    }
}

void BlockTreeStore::ReadReindexing(bool& reindexing) const
{
    reindexing = fs::exists(m_reindex_flag_file_path);
}

void BlockTreeStore::WriteReindexing(bool reindexing) const
{
    WriteFlag(m_reindex_flag_file_path, /*value=*/reindexing, /*directory_commit=*/true);
}

void BlockTreeStore::ReadLastBlockFile(int32_t& last_block_file) const
{
    LOCK(m_mutex);
    auto file{OpenFileAndVerifyHeader(m_block_files_file_path, BLOCK_FILES_FILE_MAGIC, BLOCK_FILES_FILE_VERSION)};

    constexpr uint64_t entry_size = BlockFileInfoWrapper::SERIALIZED_SIZE + sizeof(Checksum);
    const int64_t file_data_size{file.size() - BLOCK_FILES_FILE_DATA_START_POSITION};
    if (file_data_size < 0 || file_data_size % entry_size != 0) {
        throw BlockTreeStoreError("Invalid block files file data");
    }
    last_block_file = file_data_size == 0 ? 0 : file_data_size / entry_size - 1;
}

void BlockTreeStore::ReadPruned(bool& pruned) const
{
    pruned = fs::exists(m_prune_flag_file_path);
}

void BlockTreeStore::WritePruned(bool pruned) const
{
    WriteFlag(m_prune_flag_file_path, /*value=*/pruned, /*directory_commit=*/true);
}

static Checksum ExtendChecksum(Checksum checksum, std::span<const std::byte> value_data, FilePosition position)
{
    checksum = crc32c::Extend(checksum, UCharCast(value_data.data()), value_data.size());
    std::array<std::byte, sizeof(FilePosition)> position_bytes;
    WriteLE64(UCharCast(position_bytes.data()), static_cast<uint64_t>(position));
    return crc32c::Extend(checksum, UCharCast(position_bytes.data()), position_bytes.size());
}

static Checksum SingleChecksum(std::span<const std::byte> value_data, FilePosition position)
{
    return ExtendChecksum(0, value_data, position);
}

static void WriteLogFileSectionHeader(AutoFile& log_file, ValueType value_type, uint64_t record_count)
{
    log_file << static_cast<std::underlying_type_t<ValueType>>(value_type);
    log_file << record_count;
}

static std::pair<ValueType, uint64_t> ReadLogFileSectionHeader(AutoFile& log_file)
{
    const ValueType value_type{ReadValueType(log_file)};
    uint64_t record_count;
    log_file >> record_count;
    return {value_type, record_count};
}

struct LogFileRecord {
    std::vector<std::byte> m_value_buffer;
    FilePosition m_position;
    Checksum m_checksum;

    LogFileRecord(ValueType value_type) : m_value_buffer(ValueSize(value_type)) {}
};

static void ReadLogFileRecord(AutoFile& log_file, LogFileRecord& record, Checksum& rolling_checksum)
{
    log_file.read(record.m_value_buffer);
    log_file >> record.m_position;

    record.m_checksum = SingleChecksum(record.m_value_buffer, record.m_position);
    rolling_checksum = ExtendChecksum(rolling_checksum, record.m_value_buffer, record.m_position);

    Checksum stored_checksum;
    log_file >> stored_checksum;
    if (stored_checksum != record.m_checksum) {
        throw BlockTreeStoreError("Detected on-disk log file corruption: Checksum mismatch");
    }
}

template <typename Wrapper>
static void WriteLogFileRecord(AutoFile& log_file, const Wrapper& wrapper, FilePosition position, Checksum& rolling_checksum)
{
    std::array<std::byte, Wrapper::SERIALIZED_SIZE> value_buffer;
    SpanWriter{value_buffer} << wrapper;
    const Checksum checksum{SingleChecksum(value_buffer, position)};
    rolling_checksum = ExtendChecksum(rolling_checksum, value_buffer, position);
    log_file.write(value_buffer);
    log_file << position;
    log_file << checksum;
}

static void ReadDataValue(AutoFile& file, std::span<std::byte> value_buffer)
{
    const FilePosition position{file.tell()};
    file.read(value_buffer);
    Checksum checksum;
    file >> checksum;
    if (SingleChecksum(value_buffer, position) != checksum) {
        throw BlockTreeStoreError("Record data failed integrity check");
    }
}

bool BlockTreeStore::ReadBlockFileInfo(int file_index, CBlockFileInfo& info)
{
    LOCK(m_mutex);

    auto file{OpenFileAndVerifyHeader(m_block_files_file_path, BLOCK_FILES_FILE_MAGIC, BLOCK_FILES_FILE_VERSION)};
    file.seek(CalculateBlockFileInfoPosition(file_index), SEEK_SET);

    BlockFileInfoWrapper info_wrapper;
    std::array<std::byte, BlockFileInfoWrapper::SERIALIZED_SIZE> buffer;

    try {
        ReadDataValue(file, buffer);
        SpanReader{buffer} >> info_wrapper;
    } catch (std::exception&) {
        return false;
    }

    info = info_wrapper;
    return true;
}

bool BlockTreeStore::ApplyLog() const
{
    AssertLockHeld(m_mutex);

    if (!fs::exists(m_log_file_path) || !fs::exists(m_log_flag_file_path)) {
        return false;
    }

    auto log_file{OpenFileAndVerifyHeader(m_log_file_path, LOG_FILE_MAGIC, LOG_FILE_VERSION)};

    Checksum rolling_checksum = 0;
    Checksum stored_rolling_checksum = 0;
    uint32_t number_of_types = 0;

    // Do a dry run to check the integrity of the log file. This should help prevent cascading errors in case of log file corruption.
    try {
        log_file >> number_of_types;
        for (uint32_t i = 0; i < number_of_types; i++) {
            const auto [value_type, record_count] = ReadLogFileSectionHeader(log_file);
            LogFileRecord record{value_type};

            for (uint64_t j = 0; j < record_count; j++) {
                ReadLogFileRecord(log_file, record, rolling_checksum);
            }
        }

        log_file >> stored_rolling_checksum;
        if (rolling_checksum != stored_rolling_checksum) {
            throw BlockTreeStoreError("Detected on-disk log file corruption: Rolling checksum mismatch");
        }
    } catch (const std::ios_base::failure& e) {
        throw BlockTreeStoreError(strprintf("Encountered exception while checking log file: %s", e.what()));
    }

    rolling_checksum = 0;
    stored_rolling_checksum = 0;
    // Seek back to the start of the log file data, but skip reading the number of types again
    log_file.seek(LOG_FILE_DATA_START_POSITION + sizeof(number_of_types), SEEK_SET);

    // Run through the file again, but this time write it to the target data files.
    for (uint32_t i = 0; i < number_of_types; ++i) {
        const auto [value_type, record_count] = ReadLogFileSectionHeader(log_file);
        auto data_file_path{GetDataFilePath(value_type)};
        auto data_file{OpenFile(data_file_path, "rb+")};
        LogFileRecord record{value_type};

        for (uint64_t j = 0; j < record_count; ++j) {
            ReadLogFileRecord(log_file, record, rolling_checksum);

            if (data_file.tell() != record.m_position) {
                data_file.seek(record.m_position, SEEK_SET);
            }

            data_file.write(record.m_value_buffer);
            data_file << record.m_checksum;

            // TEST ONLY
            if (m_incomplete_log_apply) {
                (void)data_file.fclose();
                return false;
            }
        }

        if (!data_file.Commit()) {
            throw BlockTreeStoreError(strprintf("Failed to commit write to data file %s", PathToString(data_file_path)));
        }
        if (data_file.fclose() != 0) {
            throw BlockTreeStoreError(strprintf("Failed to close after write to data file %s", PathToString(data_file_path)));
        }
    }

    log_file >> stored_rolling_checksum;
    if (rolling_checksum != stored_rolling_checksum) {
        throw BlockTreeStoreError("Detected on-disk log file corruption: Rolling checksum mismatch");
    }

    (void)log_file.fclose();
    // Reapplying a complete log (in case of a later failure) is idempotent, so avoid an unnecessary directory commit.
    WriteFlag(m_log_flag_file_path, /*value=*/false, /*directory_commit=*/false);
    return true;
}

void BlockTreeStore::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*>>& file_infos_to_write, const std::vector<CBlockIndex*>& block_indexes_to_write)
{
    AssertLockHeld(::cs_main);
    LOCK(m_mutex);

    // If there is a complete log waiting to be applied, write that first. An incomplete log is discarded.
    // This may occur if a previous write threw an exception when writing the logged data to the .dat files.
    if (ApplyLog()) {
        LogInfo("Applied block tree store write-ahead log left over from a previous failure, potentially caused by unclean shutdown or intermittent hardware issue.");
    }

    if (file_infos_to_write.empty() && block_indexes_to_write.empty()) return;

    std::vector<std::pair<CBlockIndex*, FilePosition>> pending_header_positions;
    pending_header_positions.reserve(block_indexes_to_write.size());

    // Use a write-ahead log file that gets applied to the target files.

    { // start log_file scope
    auto log_file{OpenFile(m_log_file_path, "wb")};
    WriteMagicAndVersion(log_file, LOG_FILE_MAGIC, LOG_FILE_VERSION);
    const uint32_t log_num_types{(file_infos_to_write.empty() || block_indexes_to_write.empty()) ? 1u : 2u};
    log_file << log_num_types;

    Checksum rolling_checksum = 0;

    // Write the file_info entries to the log
    if (!file_infos_to_write.empty()) {
        WriteLogFileSectionHeader(log_file, ValueType::BLOCK_FILE_INFO, file_infos_to_write.size());
        for (const auto& [file, info] : file_infos_to_write) {
            WriteLogFileRecord(log_file, BlockFileInfoWrapper{info}, CalculateBlockFileInfoPosition(file), rolling_checksum);
        }
    }

    // TEST ONLY
    if (m_incomplete_log_write) {
        (void)log_file.fclose();
        throw std::runtime_error("failed to write file");
    }

    if (!block_indexes_to_write.empty()) {
        // Read the header data end position
        FilePosition header_data_end;
        {
            auto header_file{OpenFileAndVerifyHeader(m_header_file_path, HEADER_FILE_MAGIC, HEADER_FILE_VERSION)};
            header_data_end = header_file.size();
        }

        // Write the block_indexes_to_write data to the log
        WriteLogFileSectionHeader(log_file, ValueType::DISK_BLOCK_INDEX, block_indexes_to_write.size());
        for (CBlockIndex* block_index : block_indexes_to_write) {
            FilePosition position = block_index->header_pos == CBlockIndex::UNSET_HEADER_POS ? header_data_end : block_index->header_pos;
            CDiskBlockIndex disk_index{block_index};
            WriteLogFileRecord(log_file, DiskBlockIndexWrapper{&disk_index}, position, rolling_checksum);
            if (block_index->header_pos == CBlockIndex::UNSET_HEADER_POS) {
                pending_header_positions.emplace_back(block_index, header_data_end);
                header_data_end += DiskBlockIndexWrapper::SERIALIZED_SIZE + sizeof(Checksum);
            }
        }
    }

    // Finally write the rolling checksum and commit.
    log_file << rolling_checksum;
    if (!log_file.Commit()) {
        throw BlockTreeStoreError(strprintf("Failed to commit write to log file %s", PathToString(m_log_file_path)));
    }
    if (log_file.fclose() != 0) {
        throw BlockTreeStoreError(strprintf("Failed to close after write to log file %s", PathToString(m_log_file_path)));
    }

    } // end log_file scope

    // Write the flag indicating log file completion (which also executes CommitDirectory)
    WriteFlag(m_log_flag_file_path, /*value=*/true, /*directory_commit=*/true);

    // Once committed, apply the header positions to the index and close the file.
    for (const auto& [block_index, header_pos] : pending_header_positions) {
        block_index->header_pos = header_pos;
    }

    if (!ApplyLog()) {
        throw BlockTreeStoreError("Failed to apply write-ahead log to data files");
    }
}

bool BlockTreeStore::LoadBlockIndexGuts(
    const Consensus::Params& consensus_params,
    std::function<CBlockIndex*(const uint256&)> insert_block_index,
    const util::SignalInterrupt& interrupt)
{
    AssertLockHeld(::cs_main);
    LOCK(m_mutex);

    auto file{OpenFileAndVerifyHeader(m_header_file_path, HEADER_FILE_MAGIC, HEADER_FILE_VERSION)};

    FilePosition data_end_position = file.size();
    file.seek(HEADER_FILE_DATA_START_POSITION, SEEK_SET);

    DiskBlockIndexWrapper disk_index;
    std::array<std::byte, DiskBlockIndexWrapper::SERIALIZED_SIZE> buffer;

    while (file.tell() < data_end_position) {
        if (interrupt) return false;

        auto record_start{file.tell()};
        ReadDataValue(file, buffer);
        SpanReader{buffer} >> disk_index;

        // Construct block index object
        CBlockIndex* block_index = insert_block_index(disk_index.ConstructBlockHash());
        block_index->pprev = insert_block_index(disk_index.hashPrev);
        block_index->header_pos = record_start;
        block_index->nHeight = disk_index.nHeight;
        block_index->nFile = disk_index.nFile;
        block_index->nDataPos = disk_index.nDataPos;
        block_index->nUndoPos = disk_index.nUndoPos;
        block_index->nVersion = disk_index.nVersion;
        block_index->hashMerkleRoot = disk_index.hashMerkleRoot;
        block_index->nTime = disk_index.nTime;
        block_index->nBits = disk_index.nBits;
        block_index->nNonce = disk_index.nNonce;
        block_index->nStatus = disk_index.nStatus;
        block_index->nTx = disk_index.nTx;

        if (!CheckProofOfWork(block_index->GetBlockHash(), block_index->nBits, consensus_params)) {
            LogError("CheckProofOfWork failed: %s", block_index->ToString());
            return false;
        }
    }

    return true;
}

} // namespace kernel
