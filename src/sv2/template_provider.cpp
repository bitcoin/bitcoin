#include <sv2/template_provider.h>

#include <base58.h>
#include <crypto/hex_base.h>
#include <common/args.h>
#include <logging.h>
#include <sv2/noise.h>
#include <util/strencodings.h>
#include <util/thread.h>

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
    // Wait for the node chainstate to be ready if needed.
    auto tip{m_mining.waitTipChanged(uint256::ZERO)};
    if (m_flag_interrupt_sv2) return;
    Assert(tip.hash != uint256::ZERO);

    while (!m_flag_interrupt_sv2) {
        tip = m_mining.waitTipChanged(tip.hash, 50ms);
        bool best_block_changed{WITH_LOCK(m_tp_mutex, return m_best_prev_hash != tip.hash;)};

        // Keep m_best_prev_hash unset during IBD to avoid pushing outdated
        // templates. Except for signet, because we might be the only miner.
        if (m_mining.isInitialBlockDownload() && gArgs.GetChainType() != ChainType::SIGNET) {
            best_block_changed = false;
        }

        if (best_block_changed) {
            LOCK(m_tp_mutex);
            m_best_prev_hash = tip.hash;
            m_last_block_time = GetTime<std::chrono::seconds>();
        }

        // In a later commit we'll also push new templates based on changes to
        // the mempool, so this if condition will no longer match the one above.
        if (best_block_changed) {
            m_connman->ForEachClient([this, best_block_changed](Sv2Client& client) {
                // For newly connected clients, we call SendWork after receiving
                // CoinbaseOutputDataSize.
                if (client.m_coinbase_tx_outputs_size == 0) return;

                LOCK(this->m_tp_mutex);
                if (!SendWork(client, /*send_new_prevhash=*/best_block_changed)) {
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

void Sv2TemplateProvider::ReceivedMessage(Sv2Client& client, node::Sv2MsgType msg_type) {
    switch (msg_type)
    {
    case node::Sv2MsgType::COINBASE_OUTPUT_DATA_SIZE:
    {
        LOCK(m_tp_mutex);
        if (!SendWork(client, /*send_new_prevhash=*/true)) {
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

        std::vector<uint8_t> witness_reserve_value;
        if (!block.IsNull()) {
            auto scriptWitness = block.vtx[0]->vin[0].scriptWitness;
            if (!scriptWitness.IsNull()) {
                std::copy(scriptWitness.stack[0].begin(), scriptWitness.stack[0].end(), std::back_inserter(witness_reserve_value));
            }
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

bool Sv2TemplateProvider::SendWork(Sv2Client& client, bool send_new_prevhash)
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

    LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x71 NewTemplate id=%lu to client id=%zu\n", m_template_id, client.m_id);
    client.m_send_messages.emplace_back(new_work_set.new_template);

    if (send_new_prevhash) {
        LogPrintLevel(BCLog::SV2, BCLog::Level::Debug, "Send 0x72 SetNewPrevHash to client id=%zu\n", client.m_id);
        client.m_send_messages.emplace_back(new_work_set.prev_hash);
    }

    m_block_template_cache.insert({m_template_id, std::move(new_work_set.block_template)});

    return true;
}
