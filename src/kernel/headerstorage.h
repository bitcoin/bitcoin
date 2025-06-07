// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_HEADERSTORAGE_H
#define BITCOIN_KERNEL_HEADERSTORAGE_H

#include <chain.h>
#include <kernel/chainparams.h>
#include <util/byte_units.h>
#include <util/fs.h>

#include <cstdint>

namespace util {
class SignalInterrupt;
}

namespace kernel {

//! The layout of the headers file is as follows:
// <magic> <version> <reindex flag> <data end position> [<message start> <DiskBlockIndexWrapper>]
inline constexpr uint32_t HEADER_FILE_MAGIC{0x1d5e2eb2}; // sha256sum(BLOCK_HEADER_FILE_MAGIC)
inline constexpr uint32_t HEADER_FILE_VERSION{1};
inline constexpr uint32_t HEADER_FILE_REINDEX_FLAG_POS{8}; // after magic (4bytes) and version (4bytes)
inline constexpr uint32_t HEADER_FILE_DATA_END_POS{9};     // after magic (4bytes), version (4bytes), and reindex flag (1byte)
inline constexpr uint32_t HEADER_FILE_DATA_START_POS{17};  // after magic (4bytes), version (4bytes), reindex flag (1byte), and end pos (8bytes)
inline constexpr const char* HEADER_FILE_NAME{"headers.dat"};

//! The layout of the headers file is as follows:
// <magic> <version> <last block position> <prune flag> [<message start> <BlockFileInfoWrapper>]
inline constexpr uint32_t BLOCK_FILES_FILE_MAGIC{0x6e2e2f44}; // sha256sum(BLOCK_FILES_FILE_MAGIC)
inline constexpr uint32_t BLOCK_FILES_FILE_VERSION{1};
inline constexpr uint32_t BLOCK_FILES_LAST_BLOCK_POS{8};  // after magic (4bytes) and version (4bytes)
inline constexpr uint32_t BLOCK_FILES_PRUNE_FLAG_POS{12}; // after magic (4bytes), version (4bytes), and last block (4bytes)
inline constexpr uint32_t BLOCK_FILES_DATA_START_POS{13}; // after magic (4bytes), version (4bytes), last block (4bytes), and prune flag (1byte)
inline constexpr const char* BLOCK_FILES_FILE_NAME{"blockfiles.dat"};

class BlockTreeStoreError : public std::runtime_error
{
public:
    explicit BlockTreeStoreError(const std::string& msg) : std::runtime_error(msg) {}
};

class BlockTreeStore
{
private:
    // File path for the flat file storage
    fs::path m_header_file_path;
    fs::path m_block_files_file_path;
    const CChainParams& m_params;

    mutable Mutex m_mutex;

    void CreateHeaderFile() const;
    void CreateBlockFilesFile() const;
    void CheckMagicAndVersion() const;

public:
    BlockTreeStore(const fs::path& file_path, const CChainParams& params, bool wipe_data = false);

    void ReadReindexing(bool& reindexing) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void WriteReindexing(bool reindexing) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void ReadLastBlockFile(int32_t& last_block) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void ReadPruned(bool& pruned) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    void WritePruned(bool pruned) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

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

#endif // BITCOIN_KERNEL_HEADERSTORAGE_H
