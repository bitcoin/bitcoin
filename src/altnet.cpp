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

bool CAltstack::Start(AltLogicValidation *alt_logic)
{
    m_msgproc = alt_logic;
    interruptNet.reset();
    flagInterruptAltProc = false;

    threadWarmupDrivers = std::thread(&TraceThread<std::function<void()> >, "drivers-warmup", std::function<void()>(std::bind(&CAltstack::ThreadWarmupDrivers, this)));

    threadHandleDrivers = std::thread(&TraceThread<std::function<void()> >, "altstack-handle", std::function<void()>(std::bind(&CAltstack::ThreadHandleDrivers, this)));

    threadAltProcessing = std::thread(&TraceThread<std::function<void()> >, "altstack-processing", std::function<void()>(std::bind(&CAltstack::ThreadAltProcessing, this)));
    return true;
}

void CAltstack::Stop() {
    if (threadWarmupDrivers.joinable())
        threadWarmupDrivers.join();
    if (threadHandleDrivers.joinable())
        threadHandleDrivers.join();
    if (threadAltProcessing.joinable())
        threadAltProcessing.join();
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

void CAltstack::ThreadHandleDrivers()
{
    while (!interruptNet)
    {
        LOCK(cs_vDrivers);
        // Listen for new connections on any driver.
        for (auto&& pdriver: vDrivers) {
            if (pdriver->Listen(m_node_id)) {
                m_msgproc->InitializeNode(pdriver->GetId(), pdriver->GetCapabilities(), m_node_id);
                m_node_id++;
            }
            if (interruptNet) return;
        }

        // Send pending messages to peer hosted on any driver.
        for (auto&& pdriver: vDrivers) {
            pdriver->Flush();
            if (interruptNet) return;
        }

        // Receive messages from peer hosted on any driver.
        for (auto&& pdriver: vDrivers) {
            CAltMsg msg;
            pdriver->Receive(msg);
            LOCK(cs_vRecvMsg);
            vRecvMsg.push_back(msg);
            if (interruptNet) return;
        }
    }
}

void CAltstack::ThreadAltProcessing()
{
    while (!flagInterruptAltProc) {
        {
            LOCK(cs_vRecvMsg);
            for (auto&& msg: vRecvMsg) {
                m_msgproc->ProcessMessage(msg);
            }
            vRecvMsg.clear();
        }
        m_msgproc->SendMessage();
    }
}
