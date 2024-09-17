// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_COMMON_SOCKMAN_H
#define BITCOIN_COMMON_SOCKMAN_H

#include <netaddress.h>
#include <util/sock.h>
#include <util/translation.h>

#include <memory>
#include <vector>

/**
 * A socket manager class which handles socket operations.
 * To use this class, inherit from it and implement the pure virtual methods.
 * Handled operations:
 * - binding and listening on sockets
 */
class SockMan
{
public:
    /**
     * Bind to a new address:port, start listening and add the listen socket to `m_listen`.
     * @param[in] to Where to bind.
     * @param[out] err_msg Error string if an error occurs.
     * @retval true Success.
     * @retval false Failure, `err_msg` will be set.
     */
    bool BindAndStartListening(const CService& to, bilingual_str& err_msg);

    /**
     * Stop listening by closing all listening sockets.
     */
    void StopListening();

    /**
     * List of listening sockets.
     */
    std::vector<std::shared_ptr<Sock>> m_listen;
};

#endif // BITCOIN_COMMON_SOCKMAN_H
