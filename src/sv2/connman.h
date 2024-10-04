// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SV2_CONNMAN_H
#define BITCOIN_SV2_CONNMAN_H

#include <common/sockman.h>
#include <sv2/messages.h>
#include <sv2/transport.h>
#include <pubkey.h>

namespace {
    /*
     * Supported Stratum v2 subprotocols
     */
    static constexpr uint8_t TP_SUBPROTOCOL{0x02};

    static const std::map<uint8_t, std::string> SV2_PROTOCOL_NAMES{
    {0x02, "Template Provider"},
    };
}

struct Sv2Client
{
    /* Ephemeral identifier for debugging purposes */
    size_t m_id;

    /**
     * Transport
     */
    std::unique_ptr<Sv2Transport> m_transport;

    /**
     * Whether the client has confirmed the connection with a successful SetupConnection.
     */
    bool m_setup_connection_confirmed = false;

    /**
     * Whether the client is a candidate for disconnection.
     */
    bool m_disconnect_flag = false;

    /** Queue of messages to be sent */
    std::deque<Sv2NetMsg> m_send_messages;

    /**
     * Whether the client has received CoinbaseOutputDataSize message.
     */
    bool m_coinbase_output_data_size_recv = false;

    /**
     * Specific additional coinbase tx output size required for the client.
     */
    unsigned int m_coinbase_tx_outputs_size;

    explicit Sv2Client(size_t id, std::unique_ptr<Sv2Transport> transport) :
                       m_id{id}, m_transport{std::move(transport)} {};

    bool IsFullyConnected()
    {
        return !m_disconnect_flag && m_setup_connection_confirmed;
    }

    Sv2Client(Sv2Client&) = delete;
    Sv2Client& operator=(const Sv2Client&) = delete;
};

/**
 * Interface for sv2 message handling
 */
class Sv2EventsInterface
{
public:
    /**
    * Generic notification that a message was received. Does not include the
    * message itself.
    *
    * @param[in] client The client which we have received messages from.
    * @param[in] msg_type the message type
    */
    virtual void ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) = 0;

    virtual ~Sv2EventsInterface() = default;
};

/*
 * Handle Stratum v2 connections.
 * Currently only supports inbound connections.
 */
class Sv2Connman : SockMan
{
private:
    /** Interface to pass events up */
    Sv2EventsInterface* m_msgproc;

    /**
     * The current protocol version of stratum v2 supported by the server. Not to be confused
     * with byte value of identitying the stratum v2 subprotocol.
     */
    const uint16_t m_protocol_version = 2;

    /**
     * The currently supported optional features.
     */
    const uint16_t m_optional_features = 0;

    /**
     * The subprotocol used in setup connection messages.
     * An Sv2Connman only recognizes its own subprotocol.
     */
    const uint8_t m_subprotocol;

    CKey m_static_key;

    XOnlyPubKey m_authority_pubkey;

    std::optional<Sv2SignatureNoiseMessage> m_certificate;

    /**
     * A map of all connected stratum v2 clients.
     */
    using Clients = std::unordered_map<NodeId, std::shared_ptr<Sv2Client>>;
    Clients m_sv2_clients GUARDED_BY(m_clients_mutex);

    /**
     * Creates a socket and binds the port for new stratum v2 connections.
     */
    [[nodiscard]] bool Bind(std::string host, uint16_t port);

    void DisconnectFlagged() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex);

    /**
     * Create a `Sv2Client` object and add it to the `m_sv2_clients` member.
     * @param[in] node_id Id of the newly accepted connection.
     * @param[in] me The address and port at our side of the connection.
     * @param[in] them The address and port at the peer's side of the connection.
     * @retval true on success
     * @retval false on failure, meaning that the associated socket and node_id should be discarded
     */
    virtual bool EventNewConnectionAccepted(NodeId node_id,
                                            const CService& me,
                                            const CService& them)
                                            EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex) override;

    void EventReadyToSend(NodeId node_id, bool& cancel_recv) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex);

    virtual void EventGotData(NodeId node_id, const uint8_t* data, size_t n) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex);

    virtual void EventGotEOF(NodeId node_id) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex);

    virtual void EventGotPermanentReadError(NodeId node_id, const std::string& errmsg) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex);

    virtual void EventIOLoopCompletedForAllPeers() EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex) override;

    /**
     * Encrypt the header and message payload and send it.
     * @throws std::runtime_error if encrypting the message fails.
     */
    bool EncryptAndSendMessage(Sv2Client& client, node::Sv2NetMsg& net_msg);

    /**
     * A helper method to read and decrypt multiple Sv2NetMsgs.
     */
    std::vector<node::Sv2NetMsg> ReadAndDecryptSv2NetMsgs(Sv2Client& client, Span<std::byte> buffer);

    std::shared_ptr<Sv2Client> GetClientById(NodeId node_id) const EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex);

public:
    Sv2Connman(uint8_t subprotocol, CKey static_key, XOnlyPubKey authority_pubkey, Sv2SignatureNoiseMessage certificate) :
               m_subprotocol(subprotocol), m_static_key(static_key), m_authority_pubkey(authority_pubkey), m_certificate(certificate) {};

    ~Sv2Connman();

    Mutex m_clients_mutex;

    /**
     * Starts the Stratum v2 server and thread.
     * returns false if port is unable to bind.
     */
    [[nodiscard]] bool Start(Sv2EventsInterface* msgproc, std::string host, uint16_t port);

    /**
     * Triggered on interrupt signals to stop the main event loop in ThreadSv2Handler().
     */
    void Interrupt();

    /**
     * Tear down of the connman thread and any other necessary tear down.
     */
    void StopThreads();

    /**
     * Main handler for all received stratum v2 messages.
     */
    void ProcessSv2Message(const node::Sv2NetMsg& sv2_header, Sv2Client& client);

    using Sv2ClientFn = std::function<void(Sv2Client&)>;
    /** Perform a function on each fully connected client. */
    void ForEachClient(const Sv2ClientFn& func) EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex)
    {
        LOCK(m_clients_mutex);
        for (const auto& client : m_sv2_clients) {
            if (client.second->IsFullyConnected()) func(*client.second);
        }
    };

    /** Number of clients that are not marked for disconnection, used for tests. */
    size_t ConnectedClients() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex)
    {
        return std::count_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto& c) {
            return !c.second->m_disconnect_flag;
        });
    }

    /** Number of clients with m_setup_connection_confirmed, used for tests. */
    size_t FullyConnectedClients() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex)
    {
        return std::count_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto& c) {
            return c.second->IsFullyConnected();
        });
    }

};

#endif // BITCOIN_SV2_CONNMAN_H
