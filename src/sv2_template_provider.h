#ifndef BITCOIN_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_SV2_TEMPLATE_PROVIDER_H

#include <logging.h>
#include <node/sv2_messages.h>
#include <node/miner.h>
#include <streams.h>
#include <util/sock.h>

class ChainstateManager;
class CTxMemPool;

struct Sv2Client
{
    /**
     * Receiving and sending socket for the connected client
     */
    std::shared_ptr<Sock> m_sock;

    /**
     * Whether the client has confirmed the connection with a successful SetupConnection.
     */
    bool m_setup_connection_confirmed;

    /**
     * Whether the client is a candidate for disconnection.
     */
    bool m_disconnect_flag;

    /**
     * Whether the client has received CoinbaseOutputDataSize message.
     */
    bool m_coinbase_output_data_size_recv;

    /**
     * Specific additional coinbase tx output size required for the client.
     */
    unsigned int m_coinbase_tx_outputs_size;

    explicit Sv2Client(std::shared_ptr<Sock> sock) : m_sock{sock} {};
};

struct Sv2TemplateProviderOptions
{
    /**
     * The default listening port for the server.
     */
    uint16_t port = 8442;

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
    Clients m_sv2_clients;

    /**
     * The current best known new template id. This is incremented on each new template.
     */
    uint64_t m_template_id;

    /**
     * The best known template to send to all sv2 clients.
     */
    node::Sv2NewTemplateMsg m_best_new_template;

    /**
     * The current best known SetNewPrevHash that references the current best known
     * block hash in the network.
     */
    node::Sv2SetNewPrevHashMsg m_best_prev_hash;

    /**
     * A cache that maps ids used in NewTemplate messages and its associated block.
     */
    using BlockCache = std::map<uint64_t, std::unique_ptr<node::CBlockTemplate>>;
    BlockCache m_block_cache;

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

public:
    explicit Sv2TemplateProvider(ChainstateManager& chainman, CTxMemPool& mempool) : m_chainman{chainman}, m_mempool{mempool}
    {
        Init({});
    }

    /**
     * Starts the template provider server and thread.
     * @throws std::runtime_error if port is unable to bind.
     */
    void Start(const Sv2TemplateProviderOptions& options);

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
    void ProcessSv2Message(const node::Sv2NetHeader& sv2_header, CDataStream& ss, Sv2Client& client, const node::Sv2NewTemplateMsg& best_new_template, const node::Sv2SetNewPrevHashMsg& best_prev_hash, BlockCache& block_cache, uint64_t& template_id);

private:
    void Init(const Sv2TemplateProviderOptions& options);

    /**
     * Creates a socket and binds the port for new stratum v2 connections.
     * @throws std::runtime_error if port is unable to bind.
     */
    [[nodiscard]] std::shared_ptr<Sock> BindListenPort(uint16_t port) const;

    /**
     * The main thread for the template provider, contains an event loop handling
     * all tasks for the template provider.
     */
    void ThreadSv2Handler();

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
    [[nodiscard]] NewWorkSet BuildNewWorkSet(bool future_template, unsigned int coinbase_output_max_additional_size, uint64_t template_id);

    /**
     * Sends the best NewTemplate and SetNewPrevHash to a client.
     */
    [[nodiscard]] bool SendWork(const Sv2Client& client, const node::Sv2NewTemplateMsg& new_template, const node::Sv2SetNewPrevHashMsg& prev_hash, BlockCache& block_cache, uint64_t& template_id);

    /**
     * Generates the socket events for each Sv2Client socket and the main listening socket.
     */
    [[nodiscard]] Sock::EventsPerSock GenerateWaitSockets(const std::shared_ptr<Sock>& listen_socket, const Clients& sv2_clients) const;

    /**
     * A helper method that will serialize and send a Sv2NetMsg to an Sv2Client.
     */
    template <typename T>
    [[nodiscard]] bool Send(const Sv2Client& client, const node::Sv2NetMsg<T>& sv2_msg) {
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

        try {
            ss << sv2_msg;
        } catch (const std::exception& e) {
            LogPrintf("Error serializing Sv2NetMsg: %s\n", e.what());
            return false;
        }

        try {
            ssize_t sent = client.m_sock->Send(ss.data(), ss.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
            if (sent != static_cast<ssize_t>(ss.size())) {
                return false;
            }
        } catch (const std::exception& e) {
            LogPrintf("Error sending Sv2NetMsg: %s\n", e.what());
            return false;
        }

        return true;
    }
};

#endif // BITCOIN_SV2_TEMPLATE_PROVIDER_H
