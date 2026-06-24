// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BLOCKTREESTORAGE_H
#define BITCOIN_KERNEL_BLOCKTREESTORAGE_H

#include <kernel/cs_main.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <util/fs.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class CBlockIndex;
class uint256;

namespace Consensus {
struct Params;
}
namespace util {
class SignalInterrupt;
}

namespace kernel {

class CBlockFileInfo;

// Checksums are calculated from the serialized value and its position in the
// file. This protects against out of order data and allows the same checksum
// from the log file record to be used for the actual data files.

//! The data layout of the headers file is as follows:
//! <magic> <version> [<DiskBlockIndexWrapper> <checksum>]
inline constexpr uint32_t HEADER_FILE_MAGIC{0x1d5e2eb2}; // sha256sum("BLOCK_HEADER_FILE_MAGIC")
inline constexpr uint32_t HEADER_FILE_VERSION{1};
inline constexpr int64_t HEADER_FILE_DATA_START_POSITION{8}; // after magic (4bytes), version (4bytes)
inline constexpr const char* HEADER_FILE_NAME{"headers.dat"};

//! The flag is persisted by presence or absence of this file.
inline constexpr const char* REINDEX_FLAG_FILE_NAME{"reindex.dat"};

//! The flag is persisted by presence or absence of this file.
inline constexpr const char* PRUNE_FLAG_FILE_NAME{"prune.dat"};

//! The flag is persisted by presence or absence of this file.
//! This file is used to indicate a completed log write.
inline constexpr const char* LOG_FLAG_FILE_NAME{"log_flag.dat"};

//! The data layout of the block files file is as follows:
//! <magic> <version> [<BlockFileInfoWrapper> <checksum>]
inline constexpr uint32_t BLOCK_FILES_FILE_MAGIC{0x6e2e2f44}; // sha256sum("BLOCK_FILES_FILE_MAGIC")
inline constexpr uint32_t BLOCK_FILES_FILE_VERSION{1};
inline constexpr int64_t BLOCK_FILES_FILE_DATA_START_POSITION{8}; // after magic (4bytes), version (4bytes)
inline constexpr const char* BLOCK_FILES_FILE_NAME{"blockfiles.dat"};

//! The data layout of the log file is as follows:
//! <magic> <version> <number of types>, [<type> <number of records> [<value> <target position> <checksum>]], rolling checksum
//! uint32_t, uint32_t, uint32_t,        [uint8_t, uint64_t,         [variable size, int64_t, uint32_t]], uint32_t
//! The (type, number of records) tuple is referred to as the section header.
//! The (value, target position, checksum) tuple is referred to as a log file record.
inline constexpr uint32_t LOG_FILE_MAGIC{0xa0346f91}; // sha256sum("LOG_FILE_MAGIC")
inline constexpr uint32_t LOG_FILE_VERSION{1};
inline constexpr int64_t LOG_FILE_DATA_START_POSITION{8}; // after magic (4bytes), version (4bytes)
inline constexpr const char* LOG_FILE_NAME{"log.dat"};

enum class ValueType : uint8_t {
    BLOCK_FILE_INFO = 0,
    DISK_BLOCK_INDEX = 1,
};

class BlockTreeStoreError : public std::runtime_error
{
public:
    explicit BlockTreeStoreError(const std::string& msg) : std::runtime_error(msg) {}
};

//! Excludes other processes writing to the block tree store.
class WriterLock
{
    fs::path m_dir;

public:
    explicit WriterLock(const fs::path& dir);
    ~WriterLock();

    WriterLock(const WriterLock&) = delete;
    WriterLock& operator=(const WriterLock&) = delete;
};

class BlockTreeStore
{
public:
    enum class OpenMode {
        WRITE,
        WIPE,
        READ
    };

private:
    fs::path m_header_file_path;
    fs::path m_log_file_path;
    fs::path m_log_flag_file_path;
    fs::path m_block_files_file_path;
    fs::path m_reindex_flag_file_path;
    fs::path m_prune_flag_file_path;

    // TEST ONLY
    bool m_incomplete_log_write{false};
    bool m_incomplete_log_apply{false};

    std::optional<WriterLock> m_writer_lock;
    mutable Mutex m_mutex;
    OpenMode m_mode;

    void CheckWriteAccess() const;

    void WriteFlag(const fs::path& path, bool value, bool directory_commit) const;

    /**
     * Apply a pending write-ahead log to the data files.
     *
     * @return true if a complete log was found and applied; false if there was
     * nothing to apply, either by no log file existing, or it not being
     * complete.
     */
    [[nodiscard]] bool ApplyLog() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

public:
    BlockTreeStore(const fs::path& path, OpenMode open_mode = OpenMode::WRITE);

    void ReadReindexing(bool& reindexing) const;
    void WriteReindexing(bool reindexing) const;

    //! Block files are zero indexed. Returns 0 when there are no block files indexed yet.
    void ReadLastBlockFile(int32_t& last_block_file) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void ReadPruned(bool& pruned) const;
    void WritePruned(bool pruned) const;

    const fs::path& GetDataFilePath(ValueType value_type) const;

    // TEST ONLY
    void SetSimulateIncompleteLogWrite(bool val) { m_incomplete_log_write = val; }
    void SetSimulateIncompleteLogApply(bool val) { m_incomplete_log_apply = val; }

    void WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*>>& file_infos_to_write, const std::vector<CBlockIndex*>& block_indexes_to_write)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !m_mutex);

    [[nodiscard]] bool ReadBlockFileInfo(int file_index, CBlockFileInfo& info) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    [[nodiscard]] bool LoadBlockIndexGuts(
        const Consensus::Params& consensus_params,
        std::function<CBlockIndex*(const uint256&)> insert_block_index,
        const util::SignalInterrupt& interrupt)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !m_mutex);
};

} // namespace kernel

#endif // BITCOIN_KERNEL_BLOCKTREESTORAGE_H
