// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_TEST_UTIL_NET_H
#define WIDECOIN_TEST_UTIL_NET_H

#include <net.h>

struct ConnmanTestMsg : public CConnman {
    using CConnman::CConnman;
    void AddTestNode(CNode& node)
    {
        LOCK(cs_vNodes);
        vNodes.push_back(&node);
    }
    void ClearTestNodes()
    {
        LOCK(cs_vNodes);
        for (CNode* node : vNodes) {
            delete node;
        }
        vNodes.clear();
    }

    void ProcessMessagesOnce(CNode& node) { m_msgproc->ProcessMessages(&node, flagInterruptMsgProc); }

    void NodeReceiveMsgBytes(CNode& node, const char* pch, unsigned int nBytes, bool& complete) const;

    bool ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const;
};

#endif // WIDECOIN_TEST_UTIL_NET_H
