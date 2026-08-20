// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_TEST_COMMON_H
#define MP_TEST_COMMON_H

#include <kj/debug.h>
#include <mp/proxy-io.h>

#include <stdexcept>

namespace mp {
namespace test {

//! Default event loop log handler used by tests. Logs all messages and throws
//! on errors so calling code can assert on them.
inline void DefaultLogHandler(LogMessage log)
{
    KJ_LOG(INFO, log.level, log.message);
    if (log.level == Log::Raise) throw std::runtime_error(log.message);
}

} // namespace test
} // namespace mp

#endif // MP_TEST_COMMON_H
