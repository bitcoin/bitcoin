// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALTNET_H
#define BITCOIN_ALTNET_H

#include <threadinterrupt.h>
#include <streams.h>

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
public:
    CAltstack();
    ~CAltstack();
    bool Start();
    void Stop();
    void Interrupt();
};

#endif // BITCOIN_ALTNET_H
