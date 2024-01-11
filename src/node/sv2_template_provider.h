#ifndef BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H

#include <common/sv2_noise.h>
#include <common/sv2_messages.h>
#include <common/sv2_transport.h>
#include <logging.h>
#include <net.h>
#include <util/sock.h>
#include <util/time.h>
#include <streams.h>

class ChainstateManager;

struct Sv2Client
{
    /* Ephemeral identifier for debugging purposes */
    size_t m_id;

    /**
     * Receiving and sending socket for the connected client
     */
    std::shared_ptr<Sock> m_sock;

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

    explicit Sv2Client(size_t id, std::shared_ptr<Sock> sock, std::unique_ptr<Sv2Transport> transport) :
                       m_id{id}, m_sock{std::move(sock)}, m_transport{std::move(transport)} {};

};

struct Sv2TemplateProviderOptions
{
    /**
     * The listening port for the server.
     */
    uint16_t port;

    /**
     * The current protocol version of stratum v2 supported by the server. Not to be confused
     * with byte value of identitying the stratum v2 subprotocol.
     */
    uint16_t protocol_version = 2;

    /**
     * Optional protocol features provided by the server.
     */
    uint16_t optional_features = 0;

    /**
     * The default option for the additional space required for coinbase output.
     */
    unsigned int default_coinbase_tx_additional_output_size = 0;
};

/**
 * The main class that runs the template provider server.
 */
class Sv2TemplateProvider
{

private:
    /**
     * The template provider subprotocol used in setup connection messages. The stratum v2
     * template provider only recognizes its own subprotocol.
     */
    static constexpr uint8_t TP_SUBPROTOCOL{0x02};

    CKey m_static_key;

    std::optional<Sv2SignatureNoiseMessage> m_certificate;

    /** Get name of file to store static key */
    fs::path GetStaticKeyFile();

    /** Get name of file to store authority key */
    fs::path GetAuthorityKeyFile();

    /**
     * The main listening socket for new stratum v2 connections.
     */
    std::shared_ptr<Sock> m_listening_socket;

    /**
     * The main thread for the template provider.
     */
    std::thread m_thread_sv2_handler;

    /**
     * Signal for handling interrupts and stopping the template provider event loop.
     */
    std::atomic<bool> m_flag_interrupt_sv2{false};
    CThreadInterrupt m_interrupt_sv2;

    ChainstateManager& m_chainman;

    /**
     * A list of all connected stratum v2 clients.
     */
    using Clients = std::vector<std::unique_ptr<Sv2Client>>;
    Clients m_sv2_clients GUARDED_BY(m_clients_mutex);

    /**
     * The currently supported protocol version.
     */
    uint16_t m_protocol_version;

    /**
     * The currently supported optional features.
     */
    uint16_t m_optional_features;

    /**
     * The default additional size output required for NewTemplates.
     */
    unsigned int m_default_coinbase_tx_additional_output_size;

    /**
     * The configured port to listen for new connections.
     */
    uint16_t m_port;

public:
    Mutex m_clients_mutex;

    explicit Sv2TemplateProvider(ChainstateManager& chainman);

    ~Sv2TemplateProvider();
    /**
     * Starts the template provider server and thread.
     * returns false if port is unable to bind.
     */
    [[nodiscard]] bool Start(const Sv2TemplateProviderOptions& options);

    /**
     * Triggered on interrupt signals to stop the main event loop in ThreadSv2Handler().
     */
    void Interrupt();

    /**
     * Tear down of the template provider thread and any other necessary tear down.
     */
    void StopThreads();

    /**
     * Main handler for all received stratum v2 messages.
     */
    void ProcessSv2Message(const node::Sv2NetMsg& sv2_header, Sv2Client& client);

    /**
     *  Helper function to process incoming bytes before a session is established.
     *  Progresses a handshake or fails.
     *
     *  @throws std::runtime_error if any point of the handshake, encryption/decryption
     *  fails.
     */
    void ProcessMaybeSv2Handshake(Sv2Client& client, Span<std::byte> buffer);

private:
    void Init(const Sv2TemplateProviderOptions& options);

    /**
     * Creates a socket and binds the port for new stratum v2 connections.
     * @throws std::runtime_error if port is unable to bind.
     */
    [[nodiscard]] std::shared_ptr<Sock> BindListenPort(uint16_t port) const;

    void DisconnectFlagged() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex);

    /**
     * The main thread for the template provider, contains an event loop handling
     * all tasks for the template provider.
     */
    void ThreadSv2Handler();

    /**
     * Generates the socket events for each Sv2Client socket and the main listening socket.
     */
    [[nodiscard]] Sock::EventsPerSock GenerateWaitSockets(const std::shared_ptr<Sock>& listen_socket, const Clients& sv2_clients) const;

    /**
     * Encrypt the header and message payload and send it.
     * @throws std::runtime_error if encrypting the message fails.
     */
    bool EncryptAndSendMessage(Sv2Client& client, node::Sv2NetMsg& net_msg);

    /**
     * A helper method to read and decrypt multiple Sv2NetMsgs.
     */
    std::vector<node::Sv2NetMsg> ReadAndDecryptSv2NetMsgs(Sv2Client& client, Span<std::byte> buffer);

};

#endif // BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
