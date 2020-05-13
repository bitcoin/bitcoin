// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <alt_processing.h>

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
