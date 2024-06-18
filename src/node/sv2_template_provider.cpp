#include <node/sv2_template_provider.h>

#include <base58.h>
#include <crypto/hex_base.h>
#include <common/args.h>
#include <common/sv2_noise.h>
#include <logging.h>
#include <util/strencodings.h>
#include <util/thread.h>
#include <validation.h>

Sv2TemplateProvider::Sv2TemplateProvider(interfaces::Mining& mining) : m_mining{mining}
{
    // TODO: persist static key
    CKey static_key;
    static_key.MakeNewKey(true);

    auto authority_key{GenerateRandomKey()};

    // SRI uses base58 encoded x-only pubkeys in its configuration files
    std::array<unsigned char, 34> version_pubkey_bytes;
    version_pubkey_bytes[0] = 1;
    version_pubkey_bytes[1] = 0;
    m_authority_pubkey = XOnlyPubKey(authority_key.GetPubKey());
    std::copy(m_authority_pubkey.begin(), m_authority_pubkey.end(), version_pubkey_bytes.begin() + 2);
    LogInfo("Template Provider authority key: %s\n", EncodeBase58Check(version_pubkey_bytes));
    LogTrace(BCLog::SV2, "Authority key: %s\n", HexStr(m_authority_pubkey));

    // Generate and sign certificate
    auto now{GetTime<std::chrono::seconds>()};
    uint16_t version = 0;
    // Start validity a little bit in the past to account for clock difference
    uint32_t valid_from = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now).count()) - 3600;
    uint32_t valid_to =  std::numeric_limits<unsigned int>::max(); // 2106
    Sv2SignatureNoiseMessage certificate = Sv2SignatureNoiseMessage(version, valid_from, valid_to, XOnlyPubKey(static_key.GetPubKey()), authority_key);

    // TODO: persist certificate

    m_connman = std::make_unique<Sv2Connman>(TP_SUBPROTOCOL, static_key, m_authority_pubkey, certificate);

    // Suppress unused variable warning, result is unused.
    m_mining.getTipHash();
}

bool Sv2TemplateProvider::Start(const Sv2TemplateProviderOptions& options)
{
    m_options = options;

    if (!m_connman->Start(this, m_options.host, m_options.port)) {
        return false;
    }

    m_thread_sv2_handler = std::thread(&util::TraceThread, "sv2", [this] { ThreadSv2Handler(); });
    return true;
}

Sv2TemplateProvider::~Sv2TemplateProvider()
{
    AssertLockNotHeld(m_tp_mutex);

    m_connman->Interrupt();
    m_connman->StopThreads();

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

void Sv2TemplateProvider::ThreadSv2Handler()
{
    while (!m_flag_interrupt_sv2) {
        // TODO: handle messages
    }
}

void Sv2TemplateProvider::ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) {
    switch (msg_type)
    {
    case node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE:
    {
        LOCK(m_tp_mutex);
        // TODO: Send new template and prevout
        break;
    }
    default: {}
    }
}
