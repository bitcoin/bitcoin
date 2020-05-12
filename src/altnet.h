// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALTNET_H
#define BITCOIN_ALTNET_H

#include <threadinterrupt.h>

class CAltstack
{
private:
    uint32_t m_node_id;
    CThreadInterrupt interruptNet;
    std::atomic<bool> flagInterruptAltProc{false};
public:
    CAltstack();
    ~CAltstack();
    void Start();
    void Stop();
    void Interrupt();
}

#endif // BITCOIN_ALTNET_H
