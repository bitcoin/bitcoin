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
    return true;
}

void CAltstack::Stop() {}

void CAltstack::Interrupt() {
    flagInterruptAltProc = true;
    interruptNet();
}
