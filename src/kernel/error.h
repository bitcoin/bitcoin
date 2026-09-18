// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_ERROR_H
#define BITCOIN_KERNEL_ERROR_H

#include <uint256.h>
#include <util/fs.h>

#include <optional>
#include <string>
#include <variant>

namespace kernel {

//! Errors that occur while flushing block data to disk, reported through
//! Notifications::flushError().
enum class FlushError {
    BLOCK_FILE_FLUSH_FAILED,
    UNDO_FILE_FLUSH_FAILED,
};

//! Errors that are fatal to the operation of the node, reported through
//! Notifications::fatalError(). Each error carries the dynamic context needed
//! to describe it, so that the application can build (and translate) the
//! user-facing message itself. The kernel holds no user-facing text.
struct ActivateBestChainsFailed {
    std::string error; //!< from ActivateBestChains()
};
struct AssumeutxoDataNotFound {
    uint256 blockhash;
};
struct BlockDisconnectFailed {};
struct BlockFileCloseFailed {};
struct BlockReadFailed {};
struct BlockWriteFailed {};
struct CorruptBlockFound {};
struct DiskSpaceTooLow {};
struct FailedToStartIndexes {};
struct SnapshotChainstateDirRemovalFailed {
    fs::path dir;
};
struct SnapshotChainstateRenameFailed {
    fs::path old_path;
    fs::path new_path;
};
struct SnapshotValidationFailed {
    int height_from;
    int height_to;
    std::optional<std::string> rename_error{}; //!< from InvalidateCoinsDBOnDisk(), if it failed too
};
struct SystemErrorWhileFlushing {
    std::string what;
};
struct SystemErrorWhileLoadingExternalBlockFile {
    std::string what;
};
struct SystemErrorWhileSavingBlock {
    std::string what;
};
struct UndoDataWriteFailed {};
struct UndoFileCloseFailed {};

using FatalError = std::variant<
    ActivateBestChainsFailed,
    AssumeutxoDataNotFound,
    BlockDisconnectFailed,
    BlockFileCloseFailed,
    BlockReadFailed,
    BlockWriteFailed,
    CorruptBlockFound,
    DiskSpaceTooLow,
    FailedToStartIndexes,
    SnapshotChainstateDirRemovalFailed,
    SnapshotChainstateRenameFailed,
    SnapshotValidationFailed,
    SystemErrorWhileFlushing,
    SystemErrorWhileLoadingExternalBlockFile,
    SystemErrorWhileSavingBlock,
    UndoDataWriteFailed,
    UndoFileCloseFailed>;

} // namespace kernel

#endif // BITCOIN_KERNEL_ERROR_H
