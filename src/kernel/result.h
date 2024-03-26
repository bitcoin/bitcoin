// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_RESULT_H
#define BITCOIN_KERNEL_RESULT_H

#include <util/result.h>

//! @file Kernel result types for use with util::Result.
//!
//! Currently, many kernel functions can trigger flushes and node shutdowns at
//! low levels deep within the code. This can make it hard to reason about when
//! a particular call might trigger a flush or shutdown. Ideally more
//! information about conditions leading to flushes and shutdowns would be
//! returned so higher level code could control when they happen, but in the
//! meantime the special result types in this file are used to provide a
//! reliable indication of when they have happened after the fact.

namespace kernel {
//! Result type for functions that might trigger flushes, indiciating whether
//! flushes were skipped, performed successfully, or failed.
enum class FlushStatus { SKIPPED, SUCCESS, FAILURE };

//! Result type for functions that might trigger fatal errors and need to abort
//! the node, indicating whether a fatal error did occur.
struct AbortFailure {
    bool fatal{false};
};

template <typename SuccessType = void, typename FailureType = void>
using FlushResult = util::Result<SuccessType, FailureType, FlushStatus>;
} // namespace kernel

namespace util {
template<>
struct ResultTraits<kernel::FlushStatus>
{
    static void Update(kernel::FlushStatus& dst, kernel::FlushStatus& src)
    {
        using kernel::FlushStatus;
        if (dst == FlushStatus::FAILURE || src == FlushStatus::FAILURE) {
            dst = FlushStatus::FAILURE;
        } else if (dst == FlushStatus::SUCCESS || src == FlushStatus::SUCCESS) {
           dst = FlushStatus::SUCCESS;
        } else {
           dst = FlushStatus::SKIPPED;
        }
    }
};

template<>
struct ResultTraits<kernel::AbortFailure>
{
    static void Update(kernel::AbortFailure& dst, kernel::AbortFailure& src)
    {
        dst.fatal |= src.fatal;
    }
};
} // namespace util

#endif // BITCOIN_KERNEL_RESULT_H
