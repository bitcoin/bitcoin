// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/net.h>

#include <chainparams.h>
#include <net.h>

void ConnmanTestMsg::NodeReceiveMsgBytes(CNode& node, const char* pch, unsigned int nBytes, bool& complete) const
{
    assert(node.ReceiveMsgBytes(pch, nBytes, complete));
    if (complete) {
        size_t nSizeAdded = 0;
        auto it(node.vRecvMsg.begin());
        auto it2 = it;
        for (; it != node.vRecvMsg.end(); it2 = it++) {
            // vRecvMsg contains only completed CNetMessage
            // the single possible partially deserialized message are held by TransportDeserializer
            nSizeAdded += it->m_raw_message_size;
        }
        {
            LOCK(node.cs_vProcessMsg);
            node.vProcessMsg.splice_after(node.m_process_msg_most_recent, node.vRecvMsg, node.vRecvMsg.begin(), it);
            node.m_process_msg_most_recent = it2;
            node.m_recv_msg_most_recent = node.vRecvMsg.before_begin();
            node.nProcessQueueSize += nSizeAdded;
            node.fPauseRecv = node.nProcessQueueSize > nReceiveFloodSize;
        }
    }
}

bool ConnmanTestMsg::ReceiveMsgFrom(CNode& node, CSerializedNetMsg& ser_msg) const
{
    std::vector<unsigned char> ser_msg_header;
    node.m_serializer->prepareForTransport(ser_msg, ser_msg_header);

    bool complete;
    NodeReceiveMsgBytes(node, (const char*)ser_msg_header.data(), ser_msg_header.size(), complete);
    NodeReceiveMsgBytes(node, (const char*)ser_msg.data.data(), ser_msg.data.size(), complete);
    return complete;
}
