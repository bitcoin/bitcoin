// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/log.h>

#include <threadsafety.h>

#include <iterator>
#include <utility>

namespace util::log {

Dispatcher::CallbackHandle Dispatcher::RegisterCallback(Callback callback)
{
    StdLockGuard lock{m_mutex};
    m_callbacks.push_back(std::move(callback));
    m_callback_count.fetch_add(1, std::memory_order_release);
    return std::prev(m_callbacks.end());
}

void Dispatcher::UnregisterCallback(CallbackHandle handle)
{
    StdLockGuard lock{m_mutex};
    m_callbacks.erase(handle);
    m_callback_count.fetch_sub(1, std::memory_order_release);
}

} // namespace util::log
