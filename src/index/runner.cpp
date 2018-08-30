// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include <index/base.h>
#include <index/error.h>
#include <index/runner.h>
#include <validationinterface.h>

static std::map<BaseIndex*, IndexRunner> g_running_indexes;

IndexRunner::IndexRunner(BaseIndex* index) : m_index(index)
{
    // Need to register this ValidationInterface before running Init(), so that
    // callbacks are not missed if Init sets m_synced to true.
    RegisterValidationInterface(m_index);
    if (!m_index->Init()) {
        FatalError("%s: %s failed to initialize", __func__, m_index->GetName());
        return;
    }

    m_thread_sync = std::thread(&TraceThread<std::function<void()>>, m_index->GetName(),
                                std::bind(&BaseIndex::ThreadSync, m_index, &m_interrupt));
}

IndexRunner::~IndexRunner()
{
    UnregisterValidationInterface(m_index);

    Interrupt();
    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
}

void IndexRunner::Interrupt()
{
    m_interrupt();
}

bool StartIndex(BaseIndex* index)
{
    if (!index) return false;
    auto result = g_running_indexes.emplace(std::piecewise_construct,
                                            std::forward_as_tuple(index),
                                            std::forward_as_tuple(index));
    return result.second;
}

bool InterruptIndex(BaseIndex* index)
{
    if (!index) return false;

    auto it = g_running_indexes.find(index);
    if (it == g_running_indexes.end()) return false;

    it->second.Interrupt();
    return true;
}

bool StopIndex(BaseIndex* index)
{
    if (!index) return false;
    return g_running_indexes.erase(index);
}
