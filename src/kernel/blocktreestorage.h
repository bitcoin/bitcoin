// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BLOCKTREESTORAGE_H
#define BITCOIN_KERNEL_BLOCKTREESTORAGE_H

#include <chain.h>
#include <kernel/chainparams.h>
#include <util/byte_units.h>
#include <util/fs.h>

#include <cstdint>

namespace util {
class SignalInterrupt;
}

namespace kernel {

//! The data layout of the headers file is as follows:
//! <magic> <version> <data end position> [<DiskBlockIndexWrapper> <checksum>]
inline constexpr uint32_t HEADER_FILE_MAGIC{0x1d5e2eb2}; // sha256sum(BLOCK_HEADER_FILE_MAGIC)
inline constexpr uint32_t HEADER_FILE_VERSION{1};
inline constexpr int64_t HEADER_FILE_DATA_END_POS{8};    // after magic (4bytes), version (4bytes)
inline constexpr int64_t HEADER_FILE_DATA_START_POS{20}; // after magic (4bytes), version (4bytes), and end pos (8bytes) with its checksum (4bytes)
inline constexpr const char* HEADER_FILE_NAME{"headers.dat"};

//! The flag is persisted by presence or absence of this file.
inline constexpr const char* REINDEX_FLAG_FILE_NAME{"reindex.dat"};

//! The flag is persisted by presence or absence of this file.
inline constexpr const char* PRUNE_FLAG_FILE_NAME{"prune.dat"};

//! The data layout of the block files file is as follows:
//! <magic> <version> <last block position> [<BlockFileInfoWrapper> <checksum>]
inline constexpr uint32_t BLOCK_FILES_FILE_MAGIC{0x6e2e2f44}; // sha256sum(BLOCK_FILES_FILE_MAGIC)
inline constexpr uint32_t BLOCK_FILES_FILE_VERSION{1};
inline constexpr int64_t BLOCK_FILES_LAST_BLOCK_POS{8};  // after magic (4bytes) and version (4bytes)
inline constexpr int64_t BLOCK_FILES_DATA_START_POS{16}; // after magic (4bytes), version (4bytes), and last block (4bytes) with its checksum (4bytes)
inline constexpr const char* BLOCK_FILES_FILE_NAME{"blockfiles.dat"};

//! The data layout of the log file is as follows:
//! number of types, [type, size of type, number of entries, [entry, target position, checksum]], rolling checksum
//! uint32_t,        [uint32_t, uint32_t, uint64_t,          [variable size, int64_t, uint32_t]], uint32_t
inline constexpr const char* LOG_FILE_NAME{"log.dat"};

class BlockTreeStoreError : public std::runtime_error
{
public:
    explicit BlockTreeStoreError(const std::string& msg) : std::runtime_error(msg) {}
};

class BlockTreeStore
{
private:
    fs::path m_header_file_path;
    fs::path m_log_file_path;
    fs::path m_block_files_file_path;
    fs::path m_reindex_flag_file_path;
    fs::path m_prune_flag_file_path;

    const CChainParams m_params;

    // TEST ONLY
    bool m_incomplete_log_write{false};
    bool m_incomplete_log_apply{false};

    mutable Mutex m_mutex;

    void CreateHeaderFile() const;
    void CreateBlockFilesFile() const;
    void CheckMagicAndVersion() const;

    [[nodiscard]] bool ApplyLog() const EXCLUSIVE_LOCKS_REQUIRED(m_mutex);

public:
    BlockTreeStore(const fs::path& file_path, const CChainParams& params, bool wipe_data = false);

    void ReadReindexing(bool& reindexing) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void WriteReindexing(bool reindexing) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void ReadLastBlockFile(int32_t& last_block) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void ReadPruned(bool& pruned) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void WritePruned(bool pruned) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    const fs::path& GetDataFile(uint32_t value_type) const;

    // TEST ONLY
    void SetSimulateIncompleteLogWrite(bool val) { m_incomplete_log_write = val; }
    void SetSimulateIncompleteLogApply(bool val) { m_incomplete_log_apply = val; }

    [[nodiscard]] bool WriteBatchSync(const std::vector<std::pair<int, CBlockFileInfo*>>& fileInfo, int32_t last_file, const std::vector<CBlockIndex*>& blockinfo)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !m_mutex);

    [[nodiscard]] bool ReadBlockFileInfo(int nFile, CBlockFileInfo& info) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    [[nodiscard]] bool LoadBlockIndexGuts(
        const Consensus::Params& consensusParams,
        std::function<CBlockIndex*(const uint256&)> insertBlockIndex,
        const util::SignalInterrupt& interrupt)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !m_mutex);
};

} // namespace kernel

#endif // BITCOIN_KERNEL_BLOCKTREESTORAGE_H
