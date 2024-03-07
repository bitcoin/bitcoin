// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TIMEOFFSETS_H
#define BITCOIN_NODE_TIMEOFFSETS_H

#include <sync.h>

#include <chrono>
#include <cstddef>
#include <deque>

class TimeOffsets
{
    //! Maximum number of timeoffsets stored.
    static constexpr size_t MAX_SIZE{50};
    //! Minimum difference between system and network time for a warning to be raised.
    static constexpr std::chrono::minutes WARN_THRESHOLD{10};

    mutable Mutex m_mutex;
    /** The observed time differences between our local clock and those of our outbound peers. A
     * positive offset means our peer's clock is ahead of our local clock. */
    std::deque<std::chrono::seconds> m_offsets GUARDED_BY(m_mutex){};

public:
    /** Add a new time offset sample. */
    void Add(std::chrono::seconds offset) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Compute and return the median of the collected time offset samples. The median is returned
     * as 0 when there are less than 5 samples. */
    std::chrono::seconds Median() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Raise warnings if the median time offset exceeds the warnings threshold. Returns true if
     * warnings were raised. */
    bool WarnIfOutOfSync() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_NODE_TIMEOFFSETS_H
