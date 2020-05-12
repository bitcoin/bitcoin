// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <watchdoginterface.h>

#include <scheduler.h>
#include <logging.h>

#include <future>
#include <unordered_map>
#include <utility>

struct WatchSignalsInstance {
private:
    Mutex m_mutex;
    //! List entries consist of a callback pointer and reference count. The
    //! count is equal to the number of current executions of that entry, plus 1
    //! if it's registered. It cannot be 0 because that would imply it is
    //! unregistered and also not being executed (so shouldn't exist).
    struct ListEntry { std::shared_ptr<CWatchdogInterface> callbacks; int count = 1; };
    std::list<ListEntry> m_list GUARDED_BY(m_mutex);
    std::unordered_map<CWatchdogInterface*, std::list<ListEntry>::iterator> m_map GUARDED_BY(m_mutex);

public:
    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    SingleThreadedSchedulerClient m_schedulerClient;

    explicit WatchSignalsInstance(CScheduler *pscheduler) : m_schedulerClient(pscheduler) {}

    void Register(std::shared_ptr<CWatchdogInterface> callbacks)
    {
        LOCK(m_mutex);
        auto inserted = m_map.emplace(callbacks.get(), m_list.end());
        if (inserted.second) inserted.first->second = m_list.emplace(m_list.end());
        inserted.first->second->callbacks = std::move(callbacks);
    }

    void Unregister(CWatchdogInterface* callbacks)
    {
        LOCK(m_mutex);
        auto it = m_map.find(callbacks);
        if (it != m_map.end()) {
            if (!--it->second->count) m_list.erase(it->second);
            m_map.erase(it);
        }
    }

    //! Clear unregisters every previously registered callback, erasing every
    //! map entry. After this call, the list may still contain callbacks that
    //! are currently executing, but it will be cleared when they are done
    //! executing.
    void Clear()
    {
        LOCK(m_mutex);
        for (const auto& entry : m_map) {
            if (!--entry.second->count) m_list.erase(entry.second);
        }
        m_map.clear();
    }

    template<typename F> void Iterate(F&& f)
    {
        WAIT_LOCK(m_mutex, lock);
        for (auto it = m_list.begin(); it != m_list.end();) {
            ++it->count;
            {
                REVERSE_LOCK(lock);
                f(*it->callbacks);
            }
            it = --it->count ? std::next(it) : m_list.erase(it);
        }
    }
};

static CWatchSignals g_watch_signals;

void CWatchSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler) {
    assert(!m_internals);
    m_internals.reset(new WatchSignalsInstance(&scheduler));
}

void CWatchSignals::UnregisterBackgroundSignalScheduler() {
    m_internals.reset(nullptr);
}

void CWatchSignals::FlushBackgroundCallbacks() {
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CWatchSignals::CallbacksPending() {
    if (!m_internals) return 0;
    return m_internals->m_schedulerClient.CallbacksPending();
}

CWatchSignals& GetWatchSignals()
{
    return g_watch_signals;
}

void RegisterSharedWatchdogInterface(std::shared_ptr<CWatchdogInterface> callback) {
    // Each connection captures pwalletIn to ensure that each callback is
    // executed before pwalletIn is destroyed. For more details see #18338.
    g_watch_signals.m_internals->Register(std::move(callback));
}

void RegisterWatchdogInterface(CWatchdogInterface* callbacks)
{
    // Create a shared_ptr with a no-op deleter - CValidationInterface lifecycle
    // is managed by the caller.
    RegisterSharedWatchdogInterface({callbacks, [](CWatchdogInterface*){}});
}

void CWatchSignals::BlockHeaderAnomalie() {
    auto event = [this] {
        m_internals->Iterate([&](CWatchdogInterface& callbacks) { callbacks.BlockHeaderAnomalie(); });
    };
    m_internals->m_schedulerClient.AddToProcessQueue([=] {
        event();
    });
}
