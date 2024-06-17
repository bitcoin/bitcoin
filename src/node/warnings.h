// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_WARNINGS_H
#define BITCOIN_NODE_WARNINGS_H

#include <sync.h>
#include <util/translation.h>

#include <map>
#include <variant>
#include <vector>

class UniValue;

namespace kernel {
enum class Warning;
} // namespace kernel

namespace node {
enum class Warning {
    CLOCK_OUT_OF_SYNC,
    PRE_RELEASE_TEST_BUILD,
    FATAL_INTERNAL_ERROR,
};

/**
 * @class Warnings
 * @brief Manages warning messages within a node.
 *
 * The Warnings class provides mechanisms to set, unset, and retrieve
 * warning messages. It updates the GUI when warnings are changed.
 *
 * This class is designed to be non-copyable to ensure warnings
 * are managed centrally.
 */
class Warnings
{
    typedef std::variant<kernel::Warning, node::Warning> warning_type;

    mutable Mutex m_mutex;
    std::map<warning_type, bilingual_str> m_warnings GUARDED_BY(m_mutex);

public:
    Warnings();
    //! A warnings instance should always be passed by reference, never copied.
    Warnings(const Warnings&) = delete;
    Warnings& operator=(const Warnings&) = delete;
    /**
     * @brief Set a warning message. If a warning with the specified
     *        `id` is already active, false is returned and the new
     *        warning is ignored. If `id` does not yet exist, the
     *        warning is set, the UI is updated, and true is returned.
     *
     * @param[in]   id  Unique identifier of the warning.
     * @param[in]   message Warning message to be shown.
     *
     * @returns true if the warning was indeed set (i.e. there is no
     *          active warning with this `id`), otherwise false.
     */
    bool Set(warning_type id, bilingual_str message) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /**
     * @brief Unset a warning message. If a warning with the specified
     *        `id` is active, it is unset, the UI is updated, and true
     *        is returned. Otherwise, no warning is unset and false is
     *        returned.
     *
     * @param[in]   id  Unique identifier of the warning.
     *
     * @returns true if the warning was indeed unset (i.e. there is an
     *          active warning with this `id`), otherwise false.
     */
    bool Unset(warning_type id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    /** Return potential problems detected by the node, sorted by the
     * warning_type id */
    std::vector<bilingual_str> GetMessages() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
};

/**
 * RPC helper function that wraps warnings.GetMessages().
 *
 * Returns a UniValue::VSTR with the latest warning if use_deprecated is
 * set to true, or a UniValue::VARR with all warnings otherwise.
 */
UniValue GetWarningsForRpc(const Warnings& warnings, bool use_deprecated);
} // namespace node

#endif // BITCOIN_NODE_WARNINGS_H
