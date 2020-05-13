// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALTNET_H
#define BITCOIN_ALTNET_H

#include <alt_processing.h>
#include <drivers.h>
#include <thread>
#include <threadinterrupt.h>
#include <streams.h>
#include <sync.h>
#include <util/system.h>

#include <map>
#include <vector>

class AltLogicValidation;

/* Data payload and its origin node */
class CAltMsg {
public:
    uint32_t m_node_id;
    CDataStream m_recv; // received message data

    std::string m_command;

    CAltMsg() : m_node_id(0), m_recv(0, 0) {
        m_recv.clear();
    }
};

class CAltstack
{
private:
    uint32_t m_node_id;
    CThreadInterrupt interruptNet;
    std::atomic<bool> flagInterruptAltProc{false};

    RecursiveMutex cs_vRecvMsg;
    std::vector<CAltMsg> vRecvMsg;

    RecursiveMutex cs_vDrivers;
    std::vector<CDriver*> vDrivers GUARDED_BY(cs_vDrivers);

    RecursiveMutex cs_vNodesDriver;
    std::map<uint32_t, CDriver*> mapNodesDriver GUARDED_BY(cs_vNodesDriver);

    std::thread threadWarmupDrivers;
    std::thread threadHandleDrivers;
    std::thread threadAltProcessing;

    AltLogicValidation* m_msgproc;

    CDriver *Driver(uint32_t node_id);

public:
    CAltstack();
    ~CAltstack();
    bool Start(AltLogicValidation* alt_logic);
    void Stop();
    void Interrupt();

    /* Altstack threads */
    void ThreadWarmupDrivers();
    void ThreadHandleDrivers();
    void ThreadAltProcessing();
};

#endif // BITCOIN_ALTNET_H
