// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALT_PROCESSING_H
#define BITCOIN_ALT_PROCESSING_H

#include <altnet.h>

#include <drivers.h>
#include <sync.h>
#include <validation.h>
#include <watchdoginterface.h>

#include <map>

class CAltstack;

/* Maitains validation-specific state about nodes */
class CAltNodeState {
private:
    /* Host driver id */
    uint32_t driver_id;
    /* Capabilities supported by this peer */
    TransportCapabilities caps;
    /* Best header received from this peer */
    const CBlockIndex *pindexBestHeaderReceived;
    friend class AltLogicValidation;
public:
    CAltNodeState(uint32_t driver_id, TransportCapabilities caps) :
        driver_id(driver_id), caps(caps), pindexBestHeaderReceived(nullptr) {}
};

class AltLogicValidation final : public CWatchdogInterface {
private:
    CAltstack* const m_altstack;

    RecursiveMutex cs_vNodeState;
    std::map<uint32_t, CAltNodeState> mapNodeState GUARDED_BY(cs_vNodeState);

    CAltNodeState *State(uint32_t node_id);
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

    /**
     * Overriden from CValidationInterface *
     */
    void BlockHeaderAnomalie();
};

#endif // BITCOIN_ALT_PROCESSING_H
