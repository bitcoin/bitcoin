// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <alt_processing.h>

AltLogicValidation::AltLogicValidation(CAltstack* altstack)
    : m_altstack(altstack)
{}

bool AltLogicValidation::ProcessMessage(CAltMsg& msg) {
    return true;
}

bool AltLogicValidation::SendMessage() {
    return true;
}

void AltLogicValidation::InitializeNode(uint32_t driver_id, TransportCapabilities caps, uint32_t node_id) {}

void AltLogicValidation::FinalizeNode() {}
