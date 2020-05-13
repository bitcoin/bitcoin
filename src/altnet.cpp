// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <altnet.h>

CAltstack::CAltstack() {
    m_node_id = 0;
}

CAltstack::~CAltstack()
{
    m_node_id = 0;
    Interrupt();
    Stop();
}

bool CAltstack::Start()
{
    interruptNet.reset();
    flagInterruptAltProc = false;

    threadWarmupDrivers = std::thread(&TraceThread<std::function<void()> >, "drivers-warmup", std::function<void()>(std::bind(&CAltstack::ThreadWarmupDrivers, this)));
    return true;
}

void CAltstack::Stop() {
    if (threadWarmupDrivers.joinable())
        threadWarmupDrivers.join();
}

void CAltstack::Interrupt() {
    flagInterruptAltProc = true;
    interruptNet();
}

void CAltstack::ThreadWarmupDrivers()
{
    uint32_t id = 0;
    LOCK(cs_vDrivers);
    for (auto&& pdriver: vDrivers) {
        //TODO: CDriver::LoadAtInit() ?
        pdriver->SetId(id);
        pdriver->Warmup();
        id++;
    }
}
