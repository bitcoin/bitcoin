// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sv2/connman.h>
#include <sv2/messages.h>
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
                    client.first);
            CloseConnection(client.second->m_id);
            client.second->m_disconnect_flag = true;
        }
        DisconnectFlagged();
    }

    Interrupt();
    StopThreads();
}

bool Sv2Connman::Start(Sv2EventsInterface* msgproc, std::string host, uint16_t port)
{
    m_msgproc = msgproc;

    if (!Bind(host, port)) return false;

    SockMan::Options sockman_options;
    StartSocketsThreads(sockman_options);

    return true;
}

bool Sv2Connman::Bind(std::string host, uint16_t port)
{
    const CService addr_bind = LookupNumeric(host, port);

    bilingual_str error;
    if (!BindAndStartListening(addr_bind, error)) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Template Provider failed to bind to port %d: %s\n", port, error.original);
        return false;
    }

    LogPrintLevel(BCLog::SV2, BCLog::Level::Info, "%s listening on %s:%d\n", SV2_PROTOCOL_NAMES.at(m_subprotocol), host, port);

    return true;
}


void Sv2Connman::DisconnectFlagged()
{
    AssertLockHeld(m_clients_mutex);

    // Remove clients that are flagged for disconnection.
    auto it = m_sv2_clients.begin();
    while(it != m_sv2_clients.end()) {
        if (it->second->m_disconnect_flag) {
            it = m_sv2_clients.erase(it);
        } else {
            it++;
        }
    }
}

void Sv2Connman::EventIOLoopCompletedForAllPeers()
{
    LOCK(m_clients_mutex);
    DisconnectFlagged();
}

void Sv2Connman::Interrupt()
{
    interruptNet();
}

void Sv2Connman::StopThreads()
{
    JoinSocketsThreads();
}

std::shared_ptr<Sv2Client> Sv2Connman::GetClientById(NodeId node_id) const
{
    auto it{m_sv2_clients.find(node_id)};
    if (it != m_sv2_clients.end()) {
        return it->second;
    }
    return nullptr;
}

bool Sv2Connman::EventNewConnectionAccepted(NodeId node_id,
                                          const CService& addr_bind_,
                                          const CService& addr_)
{
    Assume(m_certificate);
    LOCK(m_clients_mutex);
    std::unique_ptr transport = std::make_unique<Sv2Transport>(m_static_key, m_certificate.value());
    auto client = std::make_shared<Sv2Client>(node_id, std::move(transport));
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "New client id=%zu connected\n", node_id);
    m_sv2_clients.emplace(node_id, std::move(client));
    return true;
}

void Sv2Connman::EventReadyToSend(NodeId node_id, bool& cancel_recv)
{
    AssertLockNotHeld(m_clients_mutex);

    auto client{WITH_LOCK(m_clients_mutex, return GetClientById(node_id);)};
    if (client == nullptr) {
        cancel_recv = true;
        return;
    }

    auto it = client->m_send_messages.begin();
    std::optional<bool> expected_more;

    size_t total_sent = 0;

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


        // We rely on the 'more' value returned by GetBytesToSend to correctly predict whether more
        // bytes are still to be sent, to correctly set the MSG_MORE flag. As a sanity check,
        // verify that the previously returned 'more' was correct.
        if (expected_more.has_value()) Assume(!data.empty() == *expected_more);
        expected_more = more;

        ssize_t sent = 0;
        std::string errmsg;

        if (!data.empty()) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Send %d bytes to client id=%zu\n",
                            data.size() - total_sent, node_id);

            sent = SendBytes(node_id, data, more, errmsg);
        }

        if (sent > 0) {
            client->m_transport->MarkBytesSent(sent);
            if (static_cast<size_t>(sent) != data.size()) {
                // could not send full message; stop sending more
                break;
            }
        } else {
            if (sent < 0) {
                LogDebug(BCLog::NET, "socket send error for peer=%d: %s\n", node_id, errmsg);
                CloseConnection(node_id);
            }
            break;
        }
    }

    // Clear messages that have been handed to transport from the queue
    client->m_send_messages.erase(client->m_send_messages.begin(), it);

    // If both receiving and (non-optimistic) sending were possible, we first attempt
    // sending. If that succeeds, but does not fully drain the send queue, do not
    // attempt to receive. This avoids needlessly queueing data if the remote peer
    // is slow at receiving data, by means of TCP flow control. We only do this when
    // sending actually succeeded to make sure progress is always made; otherwise a
    // deadlock would be possible when both sides have data to send, but neither is
    // receiving.
    //
    // TODO: decide if this is useful for Sv2
    cancel_recv = total_sent > 0; // && more;
}

void Sv2Connman::EventGotData(NodeId node_id, const uint8_t* data, size_t n)
{
    AssertLockNotHeld(m_clients_mutex);

    auto client{WITH_LOCK(m_clients_mutex, return GetClientById(node_id);)};
    if (client == nullptr) {
        return;
    }

    try {
        auto msg_ = Span(data, n);
        Span<const uint8_t> msg(reinterpret_cast<const uint8_t*>(msg_.data()), msg_.size());
        while (msg.size() > 0) {
            // absorb network data
            if (!client->m_transport->ReceivedBytes(msg)) {
                // Serious transport problem
                LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Transport problem, disconnecting client id=%zu\n",
                                client->m_id);
                CloseConnection(node_id);
                // TODO: should we even bother with this?
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
        CloseConnection(node_id);
        client->m_disconnect_flag = true;
    }

}

void Sv2Connman::EventGotEOF(NodeId node_id)
{
    auto client{WITH_LOCK(m_clients_mutex, return GetClientById(node_id);)};
    if (client == nullptr) return;
    CloseConnection(node_id);
    client->m_disconnect_flag = true;
}

void Sv2Connman::EventGotPermanentReadError(NodeId node_id, const std::string& errmsg)
{
    auto client{WITH_LOCK(m_clients_mutex, return GetClientById(node_id);)};
    if (client == nullptr) return;
    CloseConnection(node_id);
    client->m_disconnect_flag = true;
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
            CloseConnection(client.m_id);
            client.m_disconnect_flag = true;
            return;
        }

        // Disconnect a client that connects on the wrong subprotocol.
        if (setup_conn.m_protocol != m_subprotocol) {
            node::Sv2SetupConnectionErrorMsg setup_conn_err{setup_conn.m_flags, std::string{"unsupported-protocol"}};

            LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x02 SetupConnectionError to client id=%zu\n",
                          client.m_id);
            client.m_send_messages.emplace_back(setup_conn_err);

            CloseConnection(client.m_id);
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
            CloseConnection(client.m_id);
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
            CloseConnection(client.m_id);
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
            CloseConnection(client.m_id);
            client.m_disconnect_flag = true;
            return;
        }

        uint32_t max_additional_size = coinbase_output_data_size.m_coinbase_output_max_additional_size;
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "coinbase_output_max_additional_size=%d bytes\n", max_additional_size);

        if (max_additional_size > MAX_BLOCK_WEIGHT) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Received impossible CoinbaseOutputDataSize from client id=%zu: %d\n",
                          client.m_id, max_additional_size);
            CloseConnection(client.m_id);
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
