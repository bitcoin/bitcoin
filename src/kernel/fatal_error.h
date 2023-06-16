// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_FATAL_ERROR_H
#define BITCOIN_KERNEL_FATAL_ERROR_H

#include <util/result.h>

namespace kernel {

enum class FatalError {
    AcceptBlockFailed,
    BlockFileImportFailed,
    ChainstateRenameFailed,
    ConnectBestBlockFailed,
    NoChainstatePaths,
    SnapshotAlreadyValidated,
    SnapshotBaseBlockhashMismatch,
    SnapshotChainstateDirRemovalFailed,
    SnapshotHashMismatch,
    SnapshotMissingChainparams,
    SnapshotStatsFailed,
};

template <typename T>
[[nodiscard]] bool IsFatal(const util::Result<T, FatalError>& result)
{
    return !result || !result.GetErrors().empty();
}

template <typename T>
[[nodiscard]] T UnwrapFatalError(const util::Result<T, FatalError> result)
{
    assert(!IsFatal(result));
    if constexpr (std::is_void<T>::value) {
        return;
    } else {
        return result.value();
    }
}

template <typename T>
[[nodiscard]] bool CheckFatalFailure(const util::Result<T, kernel::FatalError> result, const kernel::FatalError expected_failure)
{
    return !result && result.GetFailure() == expected_failure;
}

} // namespace kernel

#endif // BITCOIN_KERNEL_FATAL_ERROR_H
