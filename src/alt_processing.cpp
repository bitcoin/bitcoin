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
        pindex = FindForkInGlobalIndex(::ChainActive(), locator);
        if (pindex)
            pindex = ::ChainActive().Next(pindex);

        std::vector<CBlock> vHeaders;
        for (; pindex; pindex = ::ChainActive().Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (pindex->GetBlockHash() == hashStop)
                break;
        }
        const CNetMsgMaker msgMaker(209);
        if (vHeaders.size() > 0)
            m_altstack->PushMessage(node->driver_id, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
        return true;
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
