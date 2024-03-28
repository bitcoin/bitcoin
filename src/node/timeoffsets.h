// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_TIMEOFFSETS_H
#define BITCOIN_NODE_TIMEOFFSETS_H

#include <sync.h>

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>

class TimeOffsets
{
private:
    static constexpr size_t N{50};
    //! Minimum difference between system and network time for a warning to be raised.
    static constexpr std::chrono::minutes m_warn_threshold{10};
    //! Log the last time a GUI warning was raised to avoid flooding user with them
    mutable std::optional<std::chrono::steady_clock::time_point> m_warning_emitted GUARDED_BY(m_mutex){};
    //! Minimum time to wait before raising a new GUI warning
    static constexpr std::chrono::minutes m_warning_wait{60};

    mutable Mutex m_mutex;
    std::deque<std::chrono::seconds> m_offsets GUARDED_BY(m_mutex){};

public:
    /** Add a new time offset sample. */
    void Add(std::chrono::seconds offset) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Compute and return the median of the collected time offset samples. */
    std::chrono::seconds Median() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Raise warnings if the median time offset exceeds the warnings threshold. Returns true if
     * warnings were raised.
    */
    bool WarnIfOutOfSync() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

#endif // BITCOIN_NODE_TIMEOFFSETS_H
