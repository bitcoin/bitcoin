#ifndef BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
#define BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H

#include <net.h>
#include <streams.h>

class ChainstateManager;

struct Sv2TemplateProviderOptions
{
    /**
     * The listening port for the server.
     */
    uint16_t port;
};

/**
 * The main class that runs the template provider server.
 */
class Sv2TemplateProvider
{

private:

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
     * The configured port to listen for new connections.
     */
    uint16_t m_port;

public:
    explicit Sv2TemplateProvider()
    {
        Init({});
    }

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

private:
    void Init(const Sv2TemplateProviderOptions& options);

    /**
     * The main thread for the template provider, contains an event loop handling
     * all tasks for the template provider.
     */
    void ThreadSv2Handler();

};


#endif // BITCOIN_NODE_SV2_TEMPLATE_PROVIDER_H
