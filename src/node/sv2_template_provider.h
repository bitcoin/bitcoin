#ifndef BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H

#include <chrono>
#include <common/sv2_noise.h>
#include <common/sv2_messages.h>
#include <common/sv2_transport.h>
#include <logging.h>
#include <net.h>
#include <node/miner.h>
#include <util/sock.h>
#include <util/time.h>
#include <streams.h>

class ChainstateManager;
class CTxMemPool;

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

    /**
     * Fees in sat paid by the last submitted template
     */
    CAmount m_latest_submitted_template_fees = 0;

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

    /**
     * The default flag for all new work.
     */
    bool default_future_templates = true;
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
    * Minimum fee delta required before submitting an updated template.
    * This may be negative.
    */
    int m_minimum_fee_delta;

    /**
     * The main thread for the template provider.
     */
    std::thread m_thread_sv2_handler;

    /**
     * Signal for handling interrupts and stopping the template provider event loop.
     */
    std::atomic<bool> m_flag_interrupt_sv2{false};
    CThreadInterrupt m_interrupt_sv2;

    /**
    * ChainstateManager and CTxMemPool are both used to build new valid blocks,
    * getting the best known block hash and checking whether the node is still
    * in IBD.
    */
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;

    /**
     * A list of all connected stratum v2 clients.
     */
    using Clients = std::vector<std::unique_ptr<Sv2Client>>;
    Clients m_sv2_clients GUARDED_BY(m_clients_mutex);

    /**
     * The most recent template id. This is incremented on creating new template,
     * which happens for each connected client.
     */
    uint64_t m_template_id GUARDED_BY(m_tp_mutex){0};

    /**
     * The current best known block hash in the network.
     */
    uint256 m_best_prev_hash GUARDED_BY(m_tp_mutex){uint256(0)};

    /** When we last saw a new block connection. Used to cache stale templates
      * for some time after this.
      */
    std::chrono::nanoseconds m_last_block_time GUARDED_BY(m_tp_mutex);

    /**
     * A cache that maps ids used in NewTemplate messages and its associated block template.
     */
    using BlockTemplateCache = std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>>;
    BlockTemplateCache m_block_template_cache GUARDED_BY(m_tp_mutex);

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
     * The default setting for sending future templates.
     */
    bool m_default_future_templates;

    /**
     * The configured port to listen for new connections.
     */
    uint16_t m_port;

public:

    Mutex m_clients_mutex;
    Mutex m_tp_mutex;

    XOnlyPubKey m_authority_pubkey;

    explicit Sv2TemplateProvider(ChainstateManager& chainman, CTxMemPool& mempool);

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
    void ProcessSv2Message(const node::Sv2NetMsg& sv2_header, Sv2Client& client) EXCLUSIVE_LOCKS_REQUIRED(!m_tp_mutex);

    /**
     *  Helper function to process incoming bytes before a session is established.
     *  Progresses a handshake or fails.
     *
     *  @throws std::runtime_error if any point of the handshake, encryption/decryption
     *  fails.
     */
    void ProcessMaybeSv2Handshake(Sv2Client& client, Span<std::byte> buffer);

    /** Number of clients that are not marked for disconnection, used for tests. */
    size_t ConnectedClients() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex)
    {
        return std::count_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto& c) {
            return !c->m_disconnect_flag;
        });
    }

    /** Number of clients with m_setup_connection_confirmed, used for tests. */
    size_t FullyConnectedClients() EXCLUSIVE_LOCKS_REQUIRED(m_clients_mutex)
    {
        return std::count_if(m_sv2_clients.begin(), m_sv2_clients.end(), [](const auto& c) {
            return !c->m_disconnect_flag && c->m_setup_connection_confirmed;
        });
    }

    /* Block templates that connected clients may be working on */
    BlockTemplateCache& GetBlockTemplates() { return m_block_template_cache; }

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
    void ThreadSv2Handler() EXCLUSIVE_LOCKS_REQUIRED(!m_clients_mutex, !m_tp_mutex);

    /**
     * NewWorkSet contains the messages matching block for valid stratum v2 work.
     */
    struct NewWorkSet
    {
        node::Sv2NewTemplateMsg new_template;
        std::unique_ptr<node::CBlockTemplate> block_template;
        node::Sv2SetNewPrevHashMsg prev_hash;
    };

    /**
     * Builds a NewWorkSet that contains the Sv2NewTemplateMsg, a new full block and a Sv2SetNewPrevHashMsg that are all linked to the same work.
     */
    [[nodiscard]] NewWorkSet BuildNewWorkSet(bool future_template, unsigned int coinbase_output_max_additional_size) EXCLUSIVE_LOCKS_REQUIRED(m_tp_mutex);

    /* Forget templates from before the last block, but with a few seconds margin. */
    void PruneBlockTemplateCache() EXCLUSIVE_LOCKS_REQUIRED(m_tp_mutex);

    /**
     * Sends the best NewTemplate and SetNewPrevHash to a client.
     */
    [[nodiscard]] bool SendWork(Sv2Client& client, bool send_new_prevhash) EXCLUSIVE_LOCKS_REQUIRED(m_tp_mutex);

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
