// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/sv2_connman.h>
#include <common/sv2_messages.h>
#include <logging.h>
#include <sync.h>
#include <util/thread.h>

using node::Sv2MsgType;

Sv2Connman::~Sv2Connman()
{
    AssertLockNotHeld(m_clients_mutex);

    {
        LOCK(m_clients_mutex);
        for (const auto& client : m_sv2_clients) {
            LogTrace(BCLog::SV2, "Disconnecting client id=%zu\n",
                    client->m_id);
            client->m_disconnect_flag = true;
        }
        DisconnectFlagged();
    }

    Interrupt();
    StopThreads();
}

bool Sv2Connman::Start(Sv2EventsInterface* msgproc, std::string host, uint16_t port)
{
    m_msgproc = msgproc;

    try {
        auto sock = BindListenPort(host, port);
        m_listening_socket = std::move(sock);
    } catch (const std::runtime_error& e) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Template Provider failed to bind to port %d: %s\n", port, e.what());
        return false;
    }

    m_thread_sv2_handler = std::thread(&util::TraceThread, "sv2connman", [this] { ThreadSv2Handler(); });
    return true;
}

std::shared_ptr<Sock> Sv2Connman::BindListenPort(std::string host, uint16_t port) const
{
    const CService addr_bind = LookupNumeric(host, port);

    auto sock = CreateSock(addr_bind.GetSAFamily(), SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        throw std::runtime_error("Sv2 Template Provider cannot create socket");
    }

    struct sockaddr_storage sockaddr;
    socklen_t len = sizeof(sockaddr);

    if (!addr_bind.GetSockAddr(reinterpret_cast<struct sockaddr*>(&sockaddr), &len)) {
        throw std::runtime_error("Sv2 Template Provider failed to get socket address");
    }

    if (sock->Bind(reinterpret_cast<struct sockaddr*>(&sockaddr), len) == SOCKET_ERROR) {
        const int nErr = WSAGetLastError();
        if (nErr == WSAEADDRINUSE) {
            throw std::runtime_error(strprintf("Unable to bind to %d on this computer. Another Stratum v2 process is probably already running.\n", port));
        }

        throw std::runtime_error(strprintf("Unable to bind to %d on this computer (bind returned error %s )\n", port, NetworkErrorString(nErr)));
    }

    constexpr int max_pending_conns{4096};
    if (sock->Listen(max_pending_conns) == SOCKET_ERROR) {
        throw std::runtime_error("Sv2 listening socket has an error listening");
    }

    LogPrintLevel(BCLog::SV2, BCLog::Level::Info, "%s listening on %s:%d\n", SV2_PROTOCOL_NAMES.at(m_subprotocol), host, port);

    return sock;
}


void Sv2Connman::DisconnectFlagged()
{
    AssertLockHeld(m_clients_mutex);

    // Remove clients that are flagged for disconnection.
    m_sv2_clients.erase(
        std::remove_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto &client) {
            return client->m_disconnect_flag;
    }), m_sv2_clients.end());
}

void Sv2Connman::ThreadSv2Handler() EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex)
{
    AssertLockNotHeld(m_clients_mutex);

    while (!m_flag_interrupt_sv2) {
        {
            LOCK(m_clients_mutex);
            DisconnectFlagged();
        }

        // Poll/Select the sockets that need handling.
        Sock::EventsPerSock events_per_sock = WITH_LOCK(m_clients_mutex, return GenerateWaitSockets(m_listening_socket, m_sv2_clients));

        constexpr auto timeout = std::chrono::milliseconds(50);
        if (!events_per_sock.begin()->first->WaitMany(timeout, events_per_sock)) {
            continue;
        }

        // Accept any new connections for sv2 clients.
        const auto listening_sock = events_per_sock.find(m_listening_socket);
        if (listening_sock != events_per_sock.end() && listening_sock->second.occurred & Sock::RECV) {
            struct sockaddr_storage sockaddr;
            socklen_t sockaddr_len = sizeof(sockaddr);

            auto sock = m_listening_socket->Accept(reinterpret_cast<struct sockaddr*>(&sockaddr), &sockaddr_len);
            if (sock) {
                Assume(m_certificate);
                LOCK(m_clients_mutex);
                std::unique_ptr transport = std::make_unique<Sv2Transport>(m_static_key, m_certificate.value());
                size_t id{m_sv2_clients.size() + 1};
                auto client = std::make_unique<Sv2Client>(id, std::move(sock), std::move(transport));
                LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "New client id=%zu connected\n", client->m_id);
                m_sv2_clients.emplace_back(std::move(client));
            }
        }

        LOCK(m_clients_mutex);
        // Process messages from and for connected sv2_clients.
        for (auto& client : m_sv2_clients) {
            bool has_received_data = false;
            bool has_error_occurred = false;

            const auto socket_it = events_per_sock.find(client->m_sock);
            if (socket_it != events_per_sock.end()) {
                has_received_data = socket_it->second.occurred & Sock::RECV;
                has_error_occurred = socket_it->second.occurred & Sock::ERR;
            }

            if (has_error_occurred) {
                LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Socket receive error, disconnecting client id=%zu\n",
                client->m_id);
                client->m_disconnect_flag = true;
                continue;
            }

            // Process message queue and any outbound bytes still held by the transport
            auto it = client->m_send_messages.begin();
            std::optional<bool> expected_more;
            while(true) {
                if (it != client->m_send_messages.end()) {
                    // If possible, move one message from the send queue to the transport.
                    // This fails when there is an existing message still being sent,
                    // or when the handshake has not yet completed.
                    //
                    // Wrap Sv2NetMsg inside CSerializedNetMsg for transport
                    CSerializedNetMsg net_msg{*it};
                    if (client->m_transport->SetMessageToSend(net_msg)) {
                        ++it;
                    }
                }

                const auto& [data, more, _m_message_type] = client->m_transport->GetBytesToSend(/*have_next_message=*/it != client->m_send_messages.end());
                size_t total_sent = 0;

                // We rely on the 'more' value returned by GetBytesToSend to correctly predict whether more
                // bytes are still to be sent, to correctly set the MSG_MORE flag. As a sanity check,
                // verify that the previously returned 'more' was correct.
                if (expected_more.has_value()) Assume(!data.empty() == *expected_more);
                expected_more = more;
                ssize_t sent = 0;

                if (!data.empty()) {
                    int flags = MSG_NOSIGNAL | MSG_DONTWAIT;
#ifdef MSG_MORE
            if (more) {
                flags |= MSG_MORE;
            }
#endif
                    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Send %d bytes to client id=%zu\n",
                                  data.size() - total_sent, client->m_id);
                    sent = client->m_sock->Send(data.data() + total_sent, data.size() - total_sent, flags);
                }
                if (sent > 0) {
                    // Notify transport that bytes have been processed.
                    client->m_transport->MarkBytesSent(sent);
                    if ((size_t)sent != data.size()) {
                        // could not send full message; stop sending more
                        break;
                    }
                } else {
                    if (sent < 0) {
                        // error
                        int nErr = WSAGetLastError();
                        if (nErr != WSAEWOULDBLOCK && nErr != WSAEMSGSIZE && nErr != WSAEINTR && nErr != WSAEINPROGRESS) {
                            LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Socket send error for  client id=%zu: %s\n",
                                          client->m_id, NetworkErrorString(nErr));
                            client->m_disconnect_flag = true;
                        }
                    }
                    break;
                }
            }
            // Clear messages that have been handed to transport from the queue
            client->m_send_messages.erase(client->m_send_messages.begin(), it);

            // Stop processing this client if something went wrong during sending
            if (client->m_disconnect_flag) break;

            if (has_received_data) {
                uint8_t bytes_received_buf[0x10000];

                const auto num_bytes_received = client->m_sock->Recv(bytes_received_buf, sizeof(bytes_received_buf), MSG_DONTWAIT);
                LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Num bytes received from client id=%zu: %d\n",
                              client->m_id, num_bytes_received);

                if (num_bytes_received <= 0) {
                    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Disconnecting client id=%zu\n",
                                  client->m_id);
                    client->m_disconnect_flag = true;
                    break;
                }

                try
                {
                    auto msg_ = Span(bytes_received_buf, num_bytes_received);
                    Span<const uint8_t> msg(reinterpret_cast<const uint8_t*>(msg_.data()), msg_.size());
                    while (msg.size() > 0) {
                        // absorb network data
                        if (!client->m_transport->ReceivedBytes(msg)) {
                            // Serious transport problem
                            LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Transport problem, disconnecting client id=%zu\n",
                                          client->m_id);
                            client->m_disconnect_flag = true;
                            break;
                        }

                        if (client->m_transport->ReceivedMessageComplete()) {
                            bool dummy_reject_message = false;
                            Sv2NetMsg msg = client->m_transport->GetReceivedMessage(std::chrono::milliseconds(0), dummy_reject_message);
                            ProcessSv2Message(msg, *client.get());
                        }
                    }
                } catch (const std::exception& e) {
                    LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received error when processing client id=%zu message: %s\n", client->m_id, e.what());
                    client->m_disconnect_flag = true;
                }
            }
        }
    }
}

Sock::EventsPerSock Sv2Connman::GenerateWaitSockets(const std::shared_ptr<Sock>& listen_socket, const Clients& sv2_clients) const
{
    Sock::EventsPerSock events_per_sock;
    events_per_sock.emplace(listen_socket, Sock::Events(Sock::RECV));

    for (const auto& client : sv2_clients) {
        if (!client->m_disconnect_flag && client->m_sock) {
            events_per_sock.emplace(client->m_sock, Sock::Events{Sock::RECV | Sock::ERR});
        }
    }

    return events_per_sock;
}

void Sv2Connman::Interrupt()
{
    m_flag_interrupt_sv2 = true;
}

void Sv2Connman::StopThreads()
{
    if (m_thread_sv2_handler.joinable()) {
        m_thread_sv2_handler.join();
    }
}

void Sv2Connman::ProcessSv2Message(const Sv2NetMsg& sv2_net_msg, Sv2Client& client)
{
    uint8_t msg_type[1] = {uint8_t(sv2_net_msg.m_msg_type)};
    LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Received 0x%s %s from client id=%zu\n",
                   // After clang-17:
                   // std::format("{:x}", uint8_t(sv2_net_msg.m_msg_type)),
                   HexStr(msg_type),
                   node::SV2_MSG_NAMES.at(sv2_net_msg.m_msg_type), client.m_id);

    DataStream ss (sv2_net_msg.m_msg);

    switch (sv2_net_msg.m_msg_type)
    {
    case Sv2MsgType::SETUP_CONNECTION:
    {
        if (client.m_setup_connection_confirmed) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Client client id=%zu connection has already been confirmed\n",
                          client.m_id);
            return;
        }

        node::Sv2SetupConnectionMsg setup_conn;
        try {
            ss >> setup_conn;
        } catch (const std::exception& e) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received invalid SetupConnection message from client id=%zu: %s\n",
                          client.m_id, e.what());
            client.m_disconnect_flag = true;
            return;
        }

        // Disconnect a client that connects on the wrong subprotocol.
        if (setup_conn.m_protocol != m_subprotocol) {
            node::Sv2SetupConnectionErrorMsg setup_conn_err{setup_conn.m_flags, std::string{"unsupported-protocol"}};

            LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x02 SetupConnectionError to client id=%zu\n",
                          client.m_id);
            client.m_send_messages.emplace_back(setup_conn_err);

            client.m_disconnect_flag = true;
            return;
        }

        // Disconnect a client if they are not running a compatible protocol version.
        if ((m_protocol_version < setup_conn.m_min_version) || (m_protocol_version > setup_conn.m_max_version)) {
            node::Sv2SetupConnectionErrorMsg setup_conn_err{setup_conn.m_flags, std::string{"protocol-version-mismatch"}};
            LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x02 SetupConnection.Error to client id=%zu\n",
                          client.m_id);
            client.m_send_messages.emplace_back(setup_conn_err);

            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received a connection from client id=%zu with incompatible protocol_versions: min_version: %d, max_version: %d\n",
                          client.m_id, setup_conn.m_min_version, setup_conn.m_max_version);
            client.m_disconnect_flag = true;
            return;
        }

        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x01 SetupConnection.Success to client id=%zu\n",
                      client.m_id);
        node::Sv2SetupConnectionSuccessMsg setup_success{m_protocol_version, m_optional_features};
        client.m_send_messages.emplace_back(setup_success);

        client.m_setup_connection_confirmed = true;

        break;
    }
    case Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE:
    {
        if (!client.m_setup_connection_confirmed) {
            client.m_disconnect_flag = true;
            return;
        }

        node::Sv2CoinbaseOutputDataSizeMsg coinbase_output_data_size;
        try {
            ss >> coinbase_output_data_size;
            client.m_coinbase_output_data_size_recv = true;
        } catch (const std::exception& e) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received invalid CoinbaseOutputDataSize message from client id=%zu: %s\n",
                          client.m_id, e.what());
            client.m_disconnect_flag = true;
            return;
        }

        uint32_t max_additional_size = coinbase_output_data_size.m_coinbase_output_max_additional_size;
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "coinbase_output_max_additional_size=%d bytes\n", max_additional_size);

        if (max_additional_size > MAX_BLOCK_WEIGHT) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received impossible CoinbaseOutputDataSize from client id=%zu: %d\n",
                          client.m_id, max_additional_size);
            client.m_disconnect_flag = true;
            return;
        }

        client.m_coinbase_tx_outputs_size = coinbase_output_data_size.m_coinbase_output_max_additional_size;

        break;
    }
    default: {
        uint8_t msg_type[1]{uint8_t(sv2_net_msg.m_msg_type)};
        LogPrintLevel(BCLog::SV2, BCLog::Level::Warning, "Received unknown message type 0x%s from client id=%zu\n",
                      HexStr(msg_type), client.m_id);
        break;
    }
    }

    m_msgproc->ReceivedMessage(client, sv2_net_msg.m_msg_type);
}
