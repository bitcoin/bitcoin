// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DRIVERS_H
#define BITCOIN_DRIVERS_H

#include <altnet.h>

#include <net.h>

#include <string>
#include <vector>

class CAltMsg;

/* Represents a transport capabilities */
class TransportCapabilities
{
public:
    /** Whether this transport can send */
    bool fSending;

    /** Whether this transport can receive */
    bool fReceiving;

    /** Whether this transport support headers */
    bool fHeaders;

    TransportCapabilities(bool fSendingIn, bool fReceivingIn, bool fHeadersIn)
        : fSending(fSendingIn),
          fReceiving(fReceivingIn),
          fHeaders(fHeadersIn)
    {}
};

/**
 * Implement a transport-specific driver to support natively its
 * processing.
 *
 * Each CDriver implementation must maintain its protocol specific
 * connnection state, control logic and peer management.
 *
 */
class CDriver {
private:
    uint32_t m_driver_id; // used to link peer back to its hosting driver
public:
    /* Warmup a driver until it's ready to service connection */
    virtual bool Warmup() = 0;

    /* Flush a batch of messages pending on all peers hosted on this
     * device
     */
    virtual bool Flush() = 0;

    /* Receive messages for all peers hosted on this device */
    //TODO: std::vector<std::tuple(node_id, msgs)>> ?
    virtual bool Receive(CAltMsg& msg) = 0;

    /* Establish incoming connections */
    virtual bool Listen(uint32_t potential_node_id) = 0;

    /* Send a message to given node_id, driver MUST not flush yet */
    virtual bool Send(CSerializedNetMsg msg) = 0;

    /* Return driver capabilities */
    virtual TransportCapabilities GetCapabilities() = 0;

    /* Set driver identifier. */
    void SetId(uint32_t driver_id) {
        m_driver_id = driver_id;
    }

    /* Return driver identifier */
    uint32_t GetId() {
        return m_driver_id;
    }
};

#endif
