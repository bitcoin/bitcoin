#ifndef BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H

#include <chrono>
#include <common/sv2_connman.h>
#include <common/sv2_messages.h>
#include <common/sv2_transport.h>
#include <interfaces/mining.h>
#include <logging.h>
#include <net.h>
#include <node/miner.h>
#include <util/sock.h>
#include <util/time.h>
#include <streams.h>

using interfaces::BlockTemplate;

struct Sv2TemplateProviderOptions
{
    /**
     * Host for the server to bind to.
     */
    std::string host{"127.0.0.1"};

    /**
     * The listening port for the server.
     */
    uint16_t port{8336};
};

/**
 * The main class that runs the template provider server.
 */
class Sv2TemplateProvider : public Sv2EventsInterface
{

private:
    /**
    * The Mining interface is used to build new valid blocks, get the best known
    * block hash and to check whether the node is still in IBD.
    */
    interfaces::Mining& m_mining;

    /*
     * The template provider subprotocol used in setup connection messages. The stratum v2
     * template provider only recognizes its own subprotocol.
     */
    static constexpr uint8_t TP_SUBPROTOCOL{0x02};

    std::unique_ptr<Sv2Connman> m_connman;

    /**
    * Configuration
    */
    Sv2TemplateProviderOptions m_options;

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
    using BlockTemplateCache = std::map<uint64_t, std::unique_ptr<BlockTemplate>>;
    BlockTemplateCache m_block_template_cache GUARDED_BY(m_tp_mutex);

public:
    explicit Sv2TemplateProvider(interfaces::Mining& mining);

    ~Sv2TemplateProvider() EXCLUSIVE_LOCKS_REQUIRED(!m_tp_mutex);

    Mutex m_tp_mutex;

    /**
     * Starts the template provider server and thread.
     * returns false if port is unable to bind.
     */
    [[nodiscard]] bool Start(const Sv2TemplateProviderOptions& options = {});

    /**
     * The main thread for the template provider, contains an event loop handling
     * all tasks for the template provider.
     */
    void ThreadSv2Handler() EXCLUSIVE_LOCKS_REQUIRED(!m_tp_mutex);

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

    // Only used for tests
    XOnlyPubKey m_authority_pubkey;

    void ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) EXCLUSIVE_LOCKS_REQUIRED(!m_tp_mutex) override;

    /* Block templates that connected clients may be working on, only used for tests */
    BlockTemplateCache& GetBlockTemplates() EXCLUSIVE_LOCKS_REQUIRED(m_tp_mutex) { return m_block_template_cache; }

private:

    /**
     * NewWorkSet contains the messages matching block for valid stratum v2 work.
     */
    struct NewWorkSet
    {
        node::Sv2NewTemplateMsg new_template;
        std::unique_ptr<BlockTemplate> block_template;
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

};

#endif // BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
