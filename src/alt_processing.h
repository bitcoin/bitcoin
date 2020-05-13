// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALT_PROCESSING_H
#define BITCOIN_ALT_PROCESSING_H

#include <altnet.h>
#include <drivers.h>

class CAltstack;

class AltLogicValidation final {
private:
    CAltstack* const m_altstack;
public:
    AltLogicValidation(CAltstack* altstack);
    /** Initialize a peer */
    void InitializeNode(uint32_t driver_id, TransportCapabilities caps, uint32_t node_id);
    /** Remove a peer due to processing and call its host driver */
    void FinalizeNode();
    /**
     * Process protocol message from a given node id
     */
    bool ProcessMessage(CAltMsg& msg);
    /**
     * Send queued protocol messages to be sent to a given node id
     */
    bool SendMessage();
};

#endif // BITCOIN_ALT_PROCESSING_H
