#include <node/sv2_template_provider.h>

#include <base58.h>
#include <consensus/merkle.h>
#include <crypto/hex_base.h>
#include <common/args.h>
#include <common/sv2_noise.h>
#include <logging.h>
#include <util/readwritefile.h>
#include <util/strencodings.h>
#include <util/thread.h>
#include <validation.h>

Sv2TemplateProvider::Sv2TemplateProvider(interfaces::Mining& mining) : m_mining{mining}
{
    // TODO: persist static key
    CKey static_key;
    try {
        AutoFile{fsbridge::fopen(GetStaticKeyFile(), "rb")} >> static_key;
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Reading cached static key from %s\n", fs::PathToString(GetStaticKeyFile()));
    } catch (const std::ios_base::failure&) {
        // File is not expected to exist the first time.
        // In the unlikely event that loading an existing key fails, create a new one.
    }
    if (!static_key.IsValid()) {
        static_key = GenerateRandomKey();
        try {
            AutoFile{fsbridge::fopen(GetStaticKeyFile(), "wb")} << static_key;
        } catch (const std::ios_base::failure&) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Error writing static key to %s\n", fs::PathToString(GetStaticKeyFile()));
            // Continue, because this is not a critical failure.
        }
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Generated static key, saved to %s\n", fs::PathToString(GetStaticKeyFile()));
    }
    LogPrintLevel(BCLog::SV2, BCLog::Level::Info, "Static key: %s\n", HexStr(static_key.GetPubKey()));

   // Generate self signed certificate using (cached) authority key
    // TODO: skip loading authoritity key if -sv2cert is used

    // Load authority key if cached
    CKey authority_key;
    try {
        AutoFile{fsbridge::fopen(GetAuthorityKeyFile(), "rb")} >> authority_key;
    } catch (const std::ios_base::failure&) {
        // File is not expected to exist the first time.
        // In the unlikely event that loading an existing key fails, create a new one.
    }
    if (!authority_key.IsValid()) {
        authority_key = GenerateRandomKey();
        try {
            AutoFile{fsbridge::fopen(GetAuthorityKeyFile(), "wb")} << authority_key;
        } catch (const std::ios_base::failure&) {
            LogPrintLevel(BCLog::SV2, BCLog::Level::Error, "Error writing authority key to %s\n", fs::PathToString(GetAuthorityKeyFile()));
            // Continue, because this is not a critical failure.
        }
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Generated authority key, saved to %s\n", fs::PathToString(GetAuthorityKeyFile()));
    }
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

    m_connman = std::make_unique<Sv2Connman>(TP_SUBPROTOCOL, static_key, m_authority_pubkey, certificate);
}

fs::path Sv2TemplateProvider::GetStaticKeyFile()
{
    return gArgs.GetDataDirNet() / "sv2_static_key";
}

fs::path Sv2TemplateProvider::GetAuthorityKeyFile()
{
    return gArgs.GetDataDirNet() / "sv2_authority_key";
}

bool Sv2TemplateProvider::Start(const Sv2TemplateProviderOptions& options)
{
    m_options = options;

    if (!m_connman->Start(this, m_options.host, m_options.port)) {
        return false;
    }

    m_thread_sv2_handler = std::thread(&util::TraceThread, "sv2", [this] { ThreadSv2Handler(); });
    m_thread_sv2_mempool_handler = std::thread(&util::TraceThread, "sv2mempool", [this] { ThreadSv2MempoolHandler(); });
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
    if (m_thread_sv2_mempool_handler.joinable()) {
        m_thread_sv2_mempool_handler.join();
    }
}

void Sv2TemplateProvider::ThreadSv2Handler()
{
    while (!m_flag_interrupt_sv2) {

        auto tip{m_mining.waitTipChanged(50ms)};
        bool best_block_changed{WITH_LOCK(m_tp_mutex, return m_best_prev_hash != tip.first;)};

        // Keep m_best_prev_hash unset during IBD to avoid pushing outdated
        // templates. Except for signet, because we might be the only miner.
        if (m_mining.isInitialBlockDownload() && gArgs.GetChainType() != ChainType::SIGNET) {
            best_block_changed = false;
        }

        if (best_block_changed) {
            {
                LOCK(m_tp_mutex);
                m_best_prev_hash = tip.first;
                m_last_block_time = GetTime<std::chrono::seconds>();
                m_template_last_update = GetTime<std::chrono::seconds>();
            }

            m_connman->ForEachClient([this, best_block_changed](Sv2Client& client) {
                // For newly connected clients, we call SendWork after receiving
                // CoinbaseOutputDataSize.
                if (client.m_coinbase_tx_outputs_size == 0) return;

                LOCK(this->m_tp_mutex);
                CAmount dummy_last_fees;
                if (!SendWork(client, /*send_new_prevhash=*/best_block_changed, dummy_last_fees)) {
                    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Disconnecting client id=%zu\n",
                                    client.m_id);
                    client.m_disconnect_flag = true;
                }
            });
        }

        LOCK(m_tp_mutex);
        PruneBlockTemplateCache();
    }
}

class Timer {
private:
    std::chrono::seconds m_interval;
    std::chrono::seconds m_last_triggered;

public:
    Timer(std::chrono::seconds interval) : m_interval(interval) {
        reset();
    }

    bool trigger() {
        auto now{GetTime<std::chrono::seconds>()};
        if (now - m_last_triggered >= m_interval) {
            m_last_triggered = now;
            return true;
        }
        return false;
    }

    void reset() {
        auto now{GetTime<std::chrono::seconds>()};
        m_last_triggered = now;
    }
};

void Sv2TemplateProvider::ThreadSv2MempoolHandler()
{
    Timer timer(m_options.fee_check_interval);

    //! Fees for the previous fee_check_interval
    CAmount fees_previous_interval{0};
    //! Fees as of the last waitFeesChanged() call
    CAmount last_fees{0};

    while (!m_flag_interrupt_sv2) {
        auto timeout{std::min(std::chrono::milliseconds(100), std::chrono::milliseconds(m_options.fee_check_interval))};
        last_fees = fees_previous_interval;
        bool tip_changed{false};
        if (!m_mining.waitFeesChanged(timeout, WITH_LOCK(m_tp_mutex, return m_best_prev_hash;), m_options.fee_delta, last_fees, tip_changed)) {
            if (tip_changed) {
                timer.reset();
                fees_previous_interval = 0;
                last_fees = 0;
            }
            continue;
        }

        // Do not send new templates more frequently than the fee check interval
        if (!timer.trigger()) continue;

        // If we never created a template, continue
        if (m_template_last_update == std::chrono::milliseconds(0)) continue;

        // TODO ensure all connected clients have had work queued up for the latest prevhash.

        // This doesn't have any effect, but it will once waitFeesChanged() updates the last_fees value.
        fees_previous_interval = last_fees;

        m_connman->ForEachClient([this, last_fees, &fees_previous_interval](Sv2Client& client) {
            // For newly connected clients, we call SendWork after receiving
            // CoinbaseOutputDataSize.
            if (client.m_coinbase_tx_outputs_size == 0) return;

            LOCK(this->m_tp_mutex);
            // fees_previous_interval is only updated if the fee increase was sufficient,
            // since waitFeesChanged doesn't actually check this yet.

            CAmount fees_before = last_fees;
            if (!SendWork(client, /*send_new_prevhash=*/false, fees_before)) {
                LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Disconnecting client id=%zu\n",
                                client.m_id);
                client.m_disconnect_flag = true;
            }

            // We don't track fees_before for individual connected clients. Pick the
            // highest value amongst all connected clients (which may vary in additional_coinbase_weight).
            if (fees_before > fees_previous_interval) fees_previous_interval = fees_before;
        });
    }
}


void Sv2TemplateProvider::ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) {
    switch (msg_type)
    {
    case node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE:
    {
        LOCK(m_tp_mutex);
        CAmount dummy_last_fees;
        if (!SendWork(client, /*send_new_prevhash=*/true, dummy_last_fees)) {
            return;
        }
        break;
    }
    default: {}
    }
}

void Sv2TemplateProvider::RequestTransactionData(Sv2Client& client, node::Sv2RequestTransactionDataMsg msg)
{
    LOCK(m_tp_mutex);
    auto cached_block = m_block_template_cache.find(msg.m_template_id);
    if (cached_block != m_block_template_cache.end()) {
        CBlock block = (*cached_block->second).getBlock();

        if (block.hashPrevBlock != m_best_prev_hash) {
            LogTrace(BCLog::SV2, "Template id=%lu prevhash=%s, tip=%s\n", msg.m_template_id, HexStr(block.hashPrevBlock), HexStr(m_best_prev_hash));
            node::Sv2RequestTransactionDataErrorMsg request_tx_data_error{msg.m_template_id, "stale-template-id"};


            LogDebug(BCLog::SV2, "Send 0x75 RequestTransactionData.Error (stale-template-id) to client id=%zu\n",
                    client.m_id);
            client.m_send_messages.emplace_back(request_tx_data_error);
            return;
        }

        std::vector<uint8_t> witness_reserve_value;
        auto scriptWitness = block.vtx[0]->vin[0].scriptWitness;
        if (!scriptWitness.IsNull()) {
            std::copy(scriptWitness.stack[0].begin(), scriptWitness.stack[0].end(), std::back_inserter(witness_reserve_value));
        }
        std::vector<CTransactionRef> txs;
        if (block.vtx.size() > 0) {
            std::copy(block.vtx.begin() + 1, block.vtx.end(), std::back_inserter(txs));
        }

        node::Sv2RequestTransactionDataSuccessMsg request_tx_data_success{msg.m_template_id, std::move(witness_reserve_value), std::move(txs)};

        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x74 RequestTransactionData.Success to client id=%zu\n",
                        client.m_id);
        client.m_send_messages.emplace_back(request_tx_data_success);
    } else {
        node::Sv2RequestTransactionDataErrorMsg request_tx_data_error{msg.m_template_id, "template-id-not-found"};

        LogDebug(BCLog::SV2, "Send 0x75 RequestTransactionData.Error (template-id-not-found: %zu) to client id=%zu\n",
                msg.m_template_id, client.m_id);
        client.m_send_messages.emplace_back(request_tx_data_error);
    }
}

void Sv2TemplateProvider::SubmitSolution(node::Sv2SubmitSolutionMsg solution)
{
        LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "version=%d, timestamp=%d, nonce=%d\n",
            solution.m_version,
            solution.m_header_timestamp,
            solution.m_header_nonce
        );

        std::unique_ptr<BlockTemplate> block_template;
        {
            // We can't hold this lock until submitSolution() because it's
            // possible that the new block arrives via the p2p network at the
            // same time. That leads to a deadlock in g_best_block_mutex.
            LOCK(m_tp_mutex);
            auto cached_block_template = m_block_template_cache.find(solution.m_template_id);
            if (cached_block_template == m_block_template_cache.end()) {
                LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Template with id=%lu is no longer in cache\n",
                solution.m_template_id);
            }
            block_template = std::move(cached_block_template->second);
        }

        block_template->submitSolution(solution.m_version, solution.m_header_timestamp, solution.m_header_nonce, solution.m_coinbase_tx);
}

Sv2TemplateProvider::NewWorkSet Sv2TemplateProvider::BuildNewWorkSet(bool future_template, unsigned int coinbase_output_max_additional_size)
{
    AssertLockHeld(m_tp_mutex);

    const auto time_start{SteadyClock::now()};
    auto block_template = m_mining.createNewBlock(CScript(), {.use_mempool = true, .coinbase_max_additional_weight = coinbase_output_max_additional_size});
    LogPrintLevel(BCLog::SV2, BCLog::Level::Trace, "Assemble template: %.2fms\n",
        Ticks<MillisecondsDouble>(SteadyClock::now() - time_start));
    CBlockHeader header{block_template->getBlockHeader()};
    node::Sv2NewTemplateMsg new_template{header,
                                         block_template->getCoinbaseTx(),
                                         block_template->getCoinbaseMerklePath(),
                                         block_template->getWitnessCommitmentIndex(),
                                         m_template_id,
                                         future_template};
    node::Sv2SetNewPrevHashMsg set_new_prev_hash{header, m_template_id};

    return NewWorkSet { new_template, std::move(block_template), set_new_prev_hash};
}

void Sv2TemplateProvider::PruneBlockTemplateCache()
{
    AssertLockHeld(m_tp_mutex);

    // Allow a few seconds for clients to submit a block
    auto recent = GetTime<std::chrono::seconds>() - std::chrono::seconds(10);
    if (m_last_block_time > recent) return;
    // If the blocks prevout is not the tip's prevout, delete it.
    uint256 prev_hash = m_best_prev_hash;
    std::erase_if(m_block_template_cache, [prev_hash] (const auto& kv) {
        if (kv.second->getBlockHeader().hashPrevBlock != prev_hash) {
            return true;
        }
        return false;
    });
}

bool Sv2TemplateProvider::SendWork(Sv2Client& client, bool send_new_prevhash, CAmount& fees_before)
{
    AssertLockHeld(m_tp_mutex);

    // The current implementation doesn't create templates for future empty
    // or speculative blocks. Despite that, we first send NewTemplate with
    // future_template set to true, followed by SetNewPrevHash. We do this
    // both when first connecting and when a new block is found.
    //
    // When the template is update to take newer mempool transactions into
    // account, we set future_template to false and don't send SetNewPrevHash.

    // TODO: reuse template_id for clients with the same m_default_coinbase_tx_additional_output_size
    ++m_template_id;
    // https://github.com/bitcoin/bitcoin/pull/30356#issuecomment-2199791658
    uint32_t additional_coinbase_weight{(client.m_coinbase_tx_outputs_size + 100 + 0 + 2) * 4};
    auto new_work_set = BuildNewWorkSet(/*future_template=*/send_new_prevhash, additional_coinbase_weight);

    if (m_best_prev_hash == uint256(0)) {
        // g_best_block is set UpdateTip(), so will be 0 when the node starts
        // and no new blocks have arrived.
        m_best_prev_hash = new_work_set.block_template->getBlockHeader().hashPrevBlock;
    }

    // Do not submit new template if the fee increase is insufficient.
    // TODO: drop this when waitFeesChanged actually checks fee_delta.
    CAmount fees = 0;
    for (CAmount fee : new_work_set.block_template->getTxFees()) {
        // Skip coinbase
        if (fee < 0) continue;
        fees += fee;
    }
    if (!send_new_prevhash && fees_before + m_options.fee_delta > fees) return true;
    fees_before = fees;

    LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x71 NewTemplate id=%lu to client id=%zu\n", m_template_id, client.m_id);
    client.m_send_messages.emplace_back(new_work_set.new_template);

    if (send_new_prevhash) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x72 SetNewPrevHash to client id=%zu\n", client.m_id);
        client.m_send_messages.emplace_back(new_work_set.prev_hash);
    }

    m_block_template_cache.insert({m_template_id, std::move(new_work_set.block_template)});

    return true;
}
