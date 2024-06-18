#ifndef BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H

#include <interfaces/mining.h>
#include <common/sv2_connman.h>
#include <common/sv2_messages.h>
#include <common/sv2_transport.h>
#include <logging.h>
#include <net.h>
#include <util/sock.h>
#include <util/time.h>
#include <streams.h>

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
    void ThreadSv2Handler();

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

    // Only used for tests
    XOnlyPubKey m_authority_pubkey;

    void ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) EXCLUSIVE_LOCKS_REQUIRED(!m_tp_mutex) override;
};

#endif // BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
