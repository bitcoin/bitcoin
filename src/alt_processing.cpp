// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <alt_processing.h>

#include <primitives/block.h>
#include <protocol.h>

AltLogicValidation::AltLogicValidation(CAltstack* altstack)
    : m_altstack(altstack)
{}

CAltNodeState *AltLogicValidation::State(uint32_t node_id) {
    std::map<uint32_t, CAltNodeState>::iterator it = mapNodeState.find(node_id);
    if (it == mapNodeState.end())
        return nullptr;
    return &it->second;
}

bool AltLogicValidation::ProcessMessage(CAltMsg& msg) {
    if (msg.m_command == NetMsgType::GETHEADERS) {

        LogPrint(BCLog::ALTSTACK, "Receive GETHEADERS from %d\n", msg.m_node_id);

        LOCK(cs_vNodeState);
        CAltNodeState *node = State(msg.m_node_id);
        // Peer ask for a reply, check receiving capability
        if (!node->caps.fReceiving)
            return false;

        CBlockLocator locator;
        uint256 hashStop;
        msg.m_recv >> locator >> hashStop;

        //TODO: implement checks
        const CBlockIndex* pindex = nullptr;
        const CBlockIndex* tip = nullptr;
        tip = ::ChainActive().Tip();
        pindex = FindForkInGlobalIndex(::ChainActive(), locator);
        LogPrint(BCLog::ALTSTACK, "Fetch header starting from %s, tip %s\n", pindex->GetBlockHeader().GetHash().ToString(), tip->GetBlockHeader().GetHash().ToString());
        if (pindex)
            pindex = ::ChainActive().Next(pindex);

        std::vector<CBlock> vHeaders;
        for (; pindex; pindex = ::ChainActive().Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (pindex->GetBlockHash() == hashStop)
                break;
        }
        LogPrint(BCLog::ALTSTACK, "Sending back %d headers\n", vHeaders.size());
        const CNetMsgMaker msgMaker(209);
        if (vHeaders.size() > 0)
            m_altstack->PushMessage(node->driver_id, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
        return true;
    }
    if (msg.m_command == NetMsgType::HEADERS) {

        LogPrint(BCLog::ALTSTACK, "Receive HEADERS from %d\n", msg.m_node_id);

        std::vector<CBlockHeader> headers;

        unsigned int nCount = ReadCompactSize(msg.m_recv);
        headers.resize(nCount);
        for (unsigned int n = 0; n < nCount; n++) {
            msg.m_recv >> headers[n];
        }
        const CBlockIndex* pindex = nullptr;
        pindex = ::ChainActive().Tip();
        LogPrint(BCLog::ALTSTACK, "Receiver header %s vs tip %s\n", headers[0].GetHash().ToString(), pindex->GetBlockHeader().GetHash().ToString());
    }
    return true;
}

bool AltLogicValidation::SendMessage() {
    return true;
}

void AltLogicValidation::InitializeNode(uint32_t driver_id, TransportCapabilities caps, uint32_t node_id) {
    CAltNodeState node(driver_id, caps);
    LOCK(cs_vNodeState);
    mapNodeState.emplace(node_id, node);
}

void AltLogicValidation::FinalizeNode() {}

void AltLogicValidation::BlockHeaderAnomalie() {
    LOCK(cs_main);
    CBlockIndex *pindexBestHeader = ::ChainActive().Tip();
    const CNetMsgMaker msgMaker(209);

    const CBlockIndex* pindex = nullptr;
    pindex = ::ChainActive().Tip();
    LogPrint(BCLog::ALTSTACK, "Block header anomalie detected - Anycasting header fetching from %s\n", pindex->GetBlockHeader().GetHash().ToString());

    // "Anycast" headers fetching
    for (std::map<uint32_t, CAltNodeState>::iterator iter = mapNodeState.begin(); iter != mapNodeState.end(); iter++)
    {
        if (iter->second.caps.fSending && iter->second.caps.fReceiving && iter->second.caps.fHeaders)
            m_altstack->PushMessage(iter->second.driver_id, msgMaker.Make(NetMsgType::GETHEADERS, ::ChainActive().GetLocator(pindexBestHeader), uint256()));
    }
}
