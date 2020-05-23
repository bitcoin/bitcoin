// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CLIGHTNING_H
#define BITCOIN_CLIGHTNING_H

#include <altnet.h>
#include <drivers.h>

class ClightningDriver final : public CDriver {
private:
    int hardcoded_port = 10309;
    const char*  hardcoded_addr = "127.0.0.1";
    int driver_socket{0};
    size_t offset{0};

    std::deque<std::vector<unsigned char>> vSendMsg;
public:
    /**
     * Overriden from CDriver
     */
    bool Warmup() override;

    /**
     * Overriden from CDriver
     */
    bool Flush() override;

    /**
     * Overriden from CDriver
     */
    bool Receive(CAltMsg& msg) override;

    /*
     * Overriden from CDriver
     */
    bool Listen(uint32_t potential_node_id) override;

    /*
     * Overriden from CDriver
     */
    bool Send(CSerializedNetMsg msg) override;

    /*
     * Overriden from CDriver
     */
    TransportCapabilities GetCapabilities() override;
};

#endif
