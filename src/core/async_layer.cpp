// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core/async_layer.h>

void AsyncLayer::Start()
{
    assert(!m_thread || !m_thread->IsActive());
    m_thread = std::unique_ptr<WorkerThread>(new WorkerThread(m_request_queue));
}

void AsyncLayer::Stop()
{
    assert(m_thread && m_thread->IsActive());
    m_thread->Terminate();
}
