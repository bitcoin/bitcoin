#include <node/sv2_template_provider.h>

#include <util/thread.h>

bool Sv2TemplateProvider::Start(const Sv2TemplateProviderOptions& options)
{
    Init(options);

    return true;
}

void Sv2TemplateProvider::Init(const Sv2TemplateProviderOptions& options)
{
    m_port = options.port;
}

Sv2TemplateProvider::~Sv2TemplateProvider()
{
    Interrupt();
    StopThreads();
}

void Sv2TemplateProvider::Interrupt()
{
    m_flag_interrupt_sv2 = true;
}

void Sv2TemplateProvider::StopThreads()
{
    if (m_thread_sv2_handler.joinable()) {
        m_thread_sv2_handler.join();
    }
}
