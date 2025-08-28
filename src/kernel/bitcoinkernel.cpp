// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BITCOINKERNEL_BUILD

#include <kernel/bitcoinkernel.h>

#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <kernel/caches.h>
#include <kernel/chainparams.h>
#include <kernel/checks.h>
#include <kernel/context.h>
#include <kernel/notifications_interface.h>
#include <kernel/warning.h>
#include <logging.h>
#include <node/blockstorage.h>
#include <node/chainstate.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <undo.h>
#include <util/fs.h>
#include <util/result.h>
#include <util/signalinterrupt.h>
#include <util/task_runner.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using util::ImmediateTaskRunner;

// Define G_TRANSLATION_FUN symbol in libbitcoinkernel library so users of the
// library aren't required to export this symbol
extern const std::function<std::string(const char*)> G_TRANSLATION_FUN{nullptr};

static const kernel::Context btck_context_static{};

struct btck_BlockTreeEntry {
    CBlockIndex* m_block_index;
};

struct btck_Block {
    std::shared_ptr<const CBlock> m_block;
};

namespace {

bool is_valid_flag_combination(unsigned int flags)
{
    if (flags & SCRIPT_VERIFY_CLEANSTACK && ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) return false;
    if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) return false;
    return true;
}

class WriterStream
{
private:
    btck_WriteBytes m_writer;
    void* m_user_data;

public:
    WriterStream(btck_WriteBytes writer, void* user_data)
        : m_writer{writer}, m_user_data{user_data} {}

    //
    // Stream subset
    //
    void write(std::span<const std::byte> src)
    {
        if (m_writer(std::data(src), src.size(), m_user_data) != 0) {
            throw std::runtime_error("Failed to write serilization data");
        }
    }

    template <typename T>
    WriterStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

BCLog::Level get_bclog_level(btck_LogLevel level)
{
    switch (level) {
    case btck_LogLevel_INFO: {
        return BCLog::Level::Info;
    }
    case btck_LogLevel_DEBUG: {
        return BCLog::Level::Debug;
    }
    case btck_LogLevel_TRACE: {
        return BCLog::Level::Trace;
    }
    }
    assert(false);
}

BCLog::LogFlags get_bclog_flag(btck_LogCategory category)
{
    switch (category) {
    case btck_LogCategory_BENCH: {
        return BCLog::LogFlags::BENCH;
    }
    case btck_LogCategory_BLOCKSTORAGE: {
        return BCLog::LogFlags::BLOCKSTORAGE;
    }
    case btck_LogCategory_COINDB: {
        return BCLog::LogFlags::COINDB;
    }
    case btck_LogCategory_LEVELDB: {
        return BCLog::LogFlags::LEVELDB;
    }
    case btck_LogCategory_MEMPOOL: {
        return BCLog::LogFlags::MEMPOOL;
    }
    case btck_LogCategory_PRUNE: {
        return BCLog::LogFlags::PRUNE;
    }
    case btck_LogCategory_RAND: {
        return BCLog::LogFlags::RAND;
    }
    case btck_LogCategory_REINDEX: {
        return BCLog::LogFlags::REINDEX;
    }
    case btck_LogCategory_VALIDATION: {
        return BCLog::LogFlags::VALIDATION;
    }
    case btck_LogCategory_KERNEL: {
        return BCLog::LogFlags::KERNEL;
    }
    case btck_LogCategory_ALL: {
        return BCLog::LogFlags::ALL;
    }
    }
    assert(false);
}

btck_SynchronizationState cast_state(SynchronizationState state)
{
    switch (state) {
    case SynchronizationState::INIT_REINDEX:
        return btck_SynchronizationState_INIT_REINDEX;
    case SynchronizationState::INIT_DOWNLOAD:
        return btck_SynchronizationState_INIT_DOWNLOAD;
    case SynchronizationState::POST_INIT:
        return btck_SynchronizationState_POST_INIT;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

btck_Warning cast_btck_warning(kernel::Warning warning)
{
    switch (warning) {
    case kernel::Warning::UNKNOWN_NEW_RULES_ACTIVATED:
        return btck_Warning_UNKNOWN_NEW_RULES_ACTIVATED;
    case kernel::Warning::LARGE_WORK_INVALID_CHAIN:
        return btck_Warning_LARGE_WORK_INVALID_CHAIN;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

class KernelNotifications : public kernel::Notifications
{
private:
    btck_NotificationInterfaceCallbacks m_cbs;

public:
    KernelNotifications(btck_NotificationInterfaceCallbacks cbs)
        : m_cbs{cbs}
    {
    }

    ~KernelNotifications()
    {
        if (m_cbs.user_data && m_cbs.user_data_destroy) {
            m_cbs.user_data_destroy(m_cbs.user_data);
        }
        m_cbs.user_data_destroy = nullptr;
        m_cbs.user_data = nullptr;
    }

    kernel::InterruptResult blockTip(SynchronizationState state, CBlockIndex& index, double verification_progress) override
    {
        if (m_cbs.block_tip) m_cbs.block_tip(m_cbs.user_data, cast_state(state), new btck_BlockTreeEntry{&index}, verification_progress);
        return {};
    }
    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override
    {
        if (m_cbs.header_tip) m_cbs.header_tip(m_cbs.user_data, cast_state(state), height, timestamp, presync ? 1 : 0);
    }
    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override
    {
        if (m_cbs.progress) m_cbs.progress(m_cbs.user_data, title.original.c_str(), title.original.length(), progress_percent, resume_possible ? 1 : 0);
    }
    void warningSet(kernel::Warning id, const bilingual_str& message) override
    {
        if (m_cbs.warning_set) m_cbs.warning_set(m_cbs.user_data, cast_btck_warning(id), message.original.c_str(), message.original.length());
    }
    void warningUnset(kernel::Warning id) override
    {
        if (m_cbs.warning_unset) m_cbs.warning_unset(m_cbs.user_data, cast_btck_warning(id));
    }
    void flushError(const bilingual_str& message) override
    {
        if (m_cbs.flush_error) m_cbs.flush_error(m_cbs.user_data, message.original.c_str(), message.original.length());
    }
    void fatalError(const bilingual_str& message) override
    {
        if (m_cbs.fatal_error) m_cbs.fatal_error(m_cbs.user_data, message.original.c_str(), message.original.length());
    }
};

class KernelValidationInterface final : public CValidationInterface
{
public:
    btck_ValidationInterfaceCallbacks m_cbs;

    explicit KernelValidationInterface(const btck_ValidationInterfaceCallbacks vi_cbs) : m_cbs{vi_cbs} {}

    ~KernelValidationInterface()
    {
        if (m_cbs.user_data && m_cbs.user_data_destroy) {
            m_cbs.user_data_destroy(m_cbs.user_data);
        }
        m_cbs.user_data = nullptr;
        m_cbs.user_data_destroy = nullptr;
    }

protected:
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& stateIn) override
    {
        if (m_cbs.block_checked) {
            m_cbs.block_checked((void*)m_cbs.user_data,
                                new btck_Block{block},
                                reinterpret_cast<const btck_BlockValidationState*>(&stateIn));
        }
    }
};

struct ContextOptions {
    mutable Mutex m_mutex;
    std::unique_ptr<const CChainParams> m_chainparams GUARDED_BY(m_mutex);
    std::shared_ptr<KernelNotifications> m_notifications GUARDED_BY(m_mutex);
    std::shared_ptr<KernelValidationInterface> m_validation_interface GUARDED_BY(m_mutex);
};

class Context
{
public:
    std::unique_ptr<kernel::Context> m_context;

    std::shared_ptr<KernelNotifications> m_notifications;

    std::unique_ptr<util::SignalInterrupt> m_interrupt;

    std::unique_ptr<ValidationSignals> m_signals;

    std::unique_ptr<const CChainParams> m_chainparams;

    std::shared_ptr<KernelValidationInterface> m_validation_interface;

    Context(const ContextOptions* options, bool& sane)
        : m_context{std::make_unique<kernel::Context>()},
          m_interrupt{std::make_unique<util::SignalInterrupt>()},
          m_signals{std::make_unique<ValidationSignals>(std::make_unique<ImmediateTaskRunner>())}
    {
        if (options) {
            LOCK(options->m_mutex);
            if (options->m_chainparams) {
                m_chainparams = std::make_unique<const CChainParams>(*options->m_chainparams);
            }
            if (options->m_notifications) {
                m_notifications = options->m_notifications;
            }
            if (options->m_validation_interface) {
                m_validation_interface = options->m_validation_interface;
                m_signals->RegisterSharedValidationInterface(m_validation_interface);
            }
        }

        if (!m_chainparams) {
            m_chainparams = CChainParams::Main();
        }
        if (!m_notifications) {
            m_notifications = std::make_shared<KernelNotifications>(btck_NotificationInterfaceCallbacks{
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
        }

        if (!kernel::SanityChecks(*m_context)) {
            sane = false;
        }
    }

    ~Context()
    {
        m_signals->UnregisterSharedValidationInterface(m_validation_interface);
    }
};

//! Helper struct to wrap the ChainstateManager-related Options
struct ChainstateManagerOptions {
    mutable Mutex m_mutex;
    ChainstateManager::Options m_chainman_options GUARDED_BY(m_mutex);
    node::BlockManager::Options m_blockman_options GUARDED_BY(m_mutex);
    std::shared_ptr<Context> m_context;
    node::ChainstateLoadOptions m_chainstate_load_options GUARDED_BY(m_mutex);

    ChainstateManagerOptions(const std::shared_ptr<Context>& context, const fs::path& data_dir, const fs::path& blocks_dir)
        : m_chainman_options{ChainstateManager::Options{
              .chainparams = *context->m_chainparams,
              .datadir = data_dir,
              .notifications = *context->m_notifications,
              .signals = context->m_signals.get()}},
          m_blockman_options{node::BlockManager::Options{
              .chainparams = *context->m_chainparams,
              .blocks_dir = blocks_dir,
              .notifications = *context->m_notifications,
              .block_tree_db_params = DBParams{
                  .path = data_dir / "blocks" / "index",
                  .cache_bytes = kernel::CacheSizes{DEFAULT_KERNEL_CACHE}.block_tree_db,
              }}},
          m_context{context},
          m_chainstate_load_options{node::ChainstateLoadOptions{}}
    {
    }
};

const BlockValidationState* cast_block_validation_state(const btck_BlockValidationState* block_validation_state)
{
    assert(block_validation_state);
    return reinterpret_cast<const BlockValidationState*>(block_validation_state);
}

} // namespace

struct btck_Transaction {
    std::shared_ptr<const CTransaction> m_tx;
};

struct btck_TransactionOutput {
    const CTxOut* m_txout;
    bool m_owned;
};

struct btck_ScriptPubkey {
    const CScript* m_script;
    bool m_owned;
};

struct btck_LoggingConnection {
    std::unique_ptr<std::list<std::function<void(const std::string&)>>::iterator> m_connection;
    void* user_data;
    std::function<void(void* user_data)> m_deleter;

    ~btck_LoggingConnection()
    {
        if (user_data && m_deleter) {
            m_deleter(user_data);
        }
    }
};

struct btck_ContextOptions {
    std::unique_ptr<ContextOptions> m_opts;
};

struct btck_Context {
    std::shared_ptr<Context> m_context;
};

struct btck_ChainParameters {
    std::unique_ptr<const CChainParams> m_params;
};

struct btck_ChainstateManagerOptions {
    std::unique_ptr<ChainstateManagerOptions> m_opts;
};

struct btck_ChainstateManager {
    std::unique_ptr<ChainstateManager> m_chainman;
    std::shared_ptr<Context> m_context;
};

struct btck_Chain {
    const CChain* m_chain;
};

struct btck_BlockSpentOutputs {
    std::shared_ptr<CBlockUndo> m_block_undo;
};

struct btck_TransactionSpentOutputs {
    const CTxUndo* m_tx_undo;
    bool m_owned;
};

struct btck_Coin {
    const Coin* m_coin;
    bool m_owned;
};

btck_Transaction* btck_transaction_create(const void* raw_transaction, size_t raw_transaction_len)
{
    try {
        DataStream stream{std::span{reinterpret_cast<const std::byte*>(raw_transaction), raw_transaction_len}};
        auto tx{std::make_shared<CTransaction>(deserialize, TX_WITH_WITNESS, stream)};
        return new btck_Transaction{std::move(tx)};
    } catch (...) {
        return nullptr;
    }
}

size_t btck_transaction_count_outputs(const btck_Transaction* transaction)
{
    return transaction->m_tx->vout.size();
}

btck_TransactionOutput* btck_transaction_get_output_at(const btck_Transaction* transaction, size_t output_index)
{
    assert(output_index < transaction->m_tx->vout.size());
    return new btck_TransactionOutput{&transaction->m_tx->vout[output_index], false};
}

size_t btck_transaction_count_inputs(const btck_Transaction* transaction)
{
    return transaction->m_tx->vin.size();
}

btck_Transaction* btck_transaction_copy(const btck_Transaction* transaction)
{
    return new btck_Transaction{transaction->m_tx};
}

int btck_transaction_to_bytes(const btck_Transaction* transaction, btck_WriteBytes writer, void* user_data)
{
    try {
        WriterStream ws{writer, user_data};
        ws << TX_WITH_WITNESS(*transaction->m_tx);
        return 0;
    } catch (...) {
        return -1;
    }
}

void btck_transaction_destroy(btck_Transaction* transaction)
{
    if (!transaction) return;
    delete transaction;
    transaction = nullptr;
}

btck_ScriptPubkey* btck_script_pubkey_create(const void* script_pubkey, size_t script_pubkey_len)
{
    auto data = std::span{reinterpret_cast<const uint8_t*>(script_pubkey), script_pubkey_len};
    return new btck_ScriptPubkey{new CScript(data.begin(), data.end()), true};
}

int btck_script_pubkey_to_bytes(const btck_ScriptPubkey* script_pubkey, btck_WriteBytes writer, void* user_data)
{
    return writer(script_pubkey->m_script->data(), script_pubkey->m_script->size(), user_data);
}

btck_ScriptPubkey* btck_script_pubkey_copy(const btck_ScriptPubkey* script_pubkey)
{
    return new btck_ScriptPubkey{new CScript(*script_pubkey->m_script), true};
}

void btck_script_pubkey_destroy(btck_ScriptPubkey* script_pubkey)
{
    if (!script_pubkey) return;
    if (script_pubkey->m_owned) {
        delete script_pubkey->m_script;
    }
    delete script_pubkey;
    script_pubkey = nullptr;
}

btck_TransactionOutput* btck_transaction_output_create(const btck_ScriptPubkey* script_pubkey, int64_t amount)
{
    const CAmount& value{amount};
    return new btck_TransactionOutput{new CTxOut(value, *script_pubkey->m_script), true};
}

btck_TransactionOutput* btck_transaction_output_copy(const btck_TransactionOutput* output)
{
    return new btck_TransactionOutput{new CTxOut{*output->m_txout}, true};
}

btck_ScriptPubkey* btck_transaction_output_get_script_pubkey(const btck_TransactionOutput* output)
{
    const auto* script_pubkey{&output->m_txout->scriptPubKey};
    return new btck_ScriptPubkey{script_pubkey, false};
}

int64_t btck_transaction_output_get_amount(const btck_TransactionOutput* output)
{
    return output->m_txout->nValue;
}

void btck_transaction_output_destroy(btck_TransactionOutput* output)
{
    if (!output) return;
    if (output->m_owned) {
        delete output->m_txout;
    }
    delete output;
    output = nullptr;
}

int btck_script_pubkey_verify(const btck_ScriptPubkey* script_pubkey,
                          const int64_t amount_,
                          const btck_Transaction* tx_to,
                          const btck_TransactionOutput** spent_outputs_, size_t spent_outputs_len,
                          const unsigned int input_index,
                          const btck_ScriptVerificationFlags flags,
                          btck_ScriptVerifyStatus* status)
{
    const CAmount amount{amount_};

    // Assert that all specified flags are part of the interface before continuing
    assert((flags & ~btck_ScriptVerificationFlags_ALL) == 0);

    if (!is_valid_flag_combination(flags)) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION;
        return 0;
    }

    if (flags & btck_ScriptVerificationFlags_TAPROOT  && spent_outputs_ == nullptr) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED;
        return 0;
    }

    const CTransaction& tx{*tx_to->m_tx};
    std::vector<CTxOut> spent_outputs;
    if (spent_outputs_ != nullptr) {
        assert(spent_outputs_len == tx.vin.size());
        spent_outputs.reserve(spent_outputs_len);
        for (size_t i = 0; i < spent_outputs_len; i++) {
            const CTxOut& tx_out{*spent_outputs_[i]->m_txout};
            spent_outputs.push_back(tx_out);
        }
    }

    assert(input_index < tx.vin.size());
    PrecomputedTransactionData txdata{tx};

    if (spent_outputs_ != nullptr && flags & btck_ScriptVerificationFlags_TAPROOT) {
        txdata.Init(tx, std::move(spent_outputs));
    }

    bool result = VerifyScript(tx.vin[input_index].scriptSig,
                        *script_pubkey->m_script,
                        &tx.vin[input_index].scriptWitness,
                        flags,
                        TransactionSignatureChecker(&tx, input_index, amount, txdata, MissingDataBehavior::FAIL),
                        nullptr);
    return result ? 1 : 0;
}

void btck_logging_set_level_category(btck_LogCategory category, btck_LogLevel level)
{
    if (category == btck_LogCategory_ALL) {
        LogInstance().SetLogLevel(get_bclog_level(level));
    }

    LogInstance().AddCategoryLogLevel(get_bclog_flag(category), get_bclog_level(level));
}

void btck_logging_enable_category(btck_LogCategory category)
{
    LogInstance().EnableCategory(get_bclog_flag(category));
}

void btck_logging_disable_category(btck_LogCategory category)
{
    LogInstance().DisableCategory(get_bclog_flag(category));
}

void btck_logging_disable()
{
    LogInstance().DisableLogging();
}

btck_LoggingConnection* btck_logging_connection_create(btck_LogCallback callback,
                                                           void* user_data,
                                                           btck_DestroyCallback user_data_destroy_callback,
                                                           const btck_LoggingOptions options)
{
    LogInstance().m_log_timestamps = options.log_timestamps;
    LogInstance().m_log_time_micros = options.log_time_micros;
    LogInstance().m_log_threadnames = options.log_threadnames;
    LogInstance().m_log_sourcelocations = options.log_sourcelocations;
    LogInstance().m_always_print_category_level = options.always_print_category_levels;

    auto connection{LogInstance().PushBackCallback([callback, user_data](const std::string& str) { callback((void*)user_data, str.c_str(), str.length()); })};

    try {
        // Only start logging if we just added the connection.
        if (LogInstance().NumConnections() == 1 && !LogInstance().StartLogging()) {
            LogError("Logger start failed.");
            LogInstance().DeleteCallback(connection);
            user_data_destroy_callback(user_data);
            return nullptr;
        }
    } catch (std::exception& e) {
        LogError("Logger start failed: %s", e.what());
        LogInstance().DeleteCallback(connection);
        user_data_destroy_callback(user_data);
        return nullptr;
    }

    LogDebug(BCLog::KERNEL, "Logger connected.");

    return new btck_LoggingConnection{std::make_unique<std::list<std::function<void(const std::string&)>>::iterator>(connection), user_data, user_data_destroy_callback};
}

void btck_logging_connection_destroy(btck_LoggingConnection* connection)
{
    if (!connection) {
        return;
    }

    LogDebug(BCLog::KERNEL, "Logger disconnected.");
    LogInstance().DeleteCallback(*connection->m_connection);
    delete connection;

    // Switch back to buffering by calling DisconnectTestLogger if the
    // connection that was just removed was the last one.
    if (!LogInstance().Enabled()) {
        LogInstance().DisconnectTestLogger();
    }
    connection = nullptr;
}

btck_ChainParameters* btck_chain_parameters_create(const btck_ChainType chain_type)
{
    switch (chain_type) {
    case btck_ChainType_MAINNET: {
        return new btck_ChainParameters{CChainParams::Main()};
    }
    case btck_ChainType_TESTNET: {
        return new btck_ChainParameters{CChainParams::TestNet()};
    }
    case btck_ChainType_TESTNET_4: {
        return new btck_ChainParameters{CChainParams::TestNet4()};
    }
    case btck_ChainType_SIGNET: {
        return new btck_ChainParameters{CChainParams::SigNet({})};
    }
    case btck_ChainType_REGTEST: {
        return new btck_ChainParameters{CChainParams::RegTest({})};
    }
    }
    assert(false);
}

void btck_chain_parameters_destroy(btck_ChainParameters* chain_parameters)
{
    if (!chain_parameters) return;
    delete chain_parameters;
    chain_parameters = nullptr;
}

btck_ContextOptions* btck_context_options_create()
{
    return new btck_ContextOptions{std::make_unique<ContextOptions>()};
}

void btck_context_options_set_chainparams(btck_ContextOptions* options, const btck_ChainParameters* chain_parameters)
{
    // Copy the chainparams, so the caller can free it again
    LOCK(options->m_opts->m_mutex);
    options->m_opts->m_chainparams = std::make_unique<const CChainParams>(*chain_parameters->m_params);
}

void btck_context_options_set_notifications(btck_ContextOptions* options, btck_NotificationInterfaceCallbacks notifications)
{
    // The KernelNotifications are copy-initialized, so the caller can free them again.
    LOCK(options->m_opts->m_mutex);
    options->m_opts->m_notifications = std::make_shared<KernelNotifications>(notifications);
}

void btck_context_options_set_validation_interface(btck_ContextOptions* options, btck_ValidationInterfaceCallbacks vi_cbs)
{
    LOCK(options->m_opts->m_mutex);
    options->m_opts->m_validation_interface = std::make_shared<KernelValidationInterface>(vi_cbs);
}

void btck_context_options_destroy(btck_ContextOptions* options)
{
    if (!options) return;
    delete options;
    options = nullptr;
}

btck_Context* btck_context_create(const btck_ContextOptions* options)
{
    bool sane{true};
    auto context{std::make_shared<Context>(options->m_opts.get(), sane)};
    if (!sane) {
        LogError("Kernel context sanity check failed.");
        return nullptr;
    }
    return new btck_Context{std::move(context)};
}

int btck_context_interrupt(btck_Context* context)
{
    return (*context->m_context->m_interrupt)() ? 0 : -1;
}

void btck_context_destroy(btck_Context* context)
{
    if (!context) return;
    delete context;
    context = nullptr;
}

btck_BlockTreeEntry* btck_block_tree_entry_get_previous(const btck_BlockTreeEntry* entry)
{
    if (!entry->m_block_index->pprev) {
        LogInfo("Genesis block has no previous.");
        return nullptr;
    }

    return new btck_BlockTreeEntry{entry->m_block_index->pprev};
}

void btck_block_tree_entry_destroy(btck_BlockTreeEntry* block_tree_entry)
{
    if (!block_tree_entry) return;
    delete block_tree_entry;
    block_tree_entry = nullptr;
}

btck_ValidationMode btck_block_validation_state_get_validation_mode(const btck_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = *cast_block_validation_state(block_validation_state_);
    if (block_validation_state.IsValid()) return btck_ValidationMode_VALID;
    if (block_validation_state.IsInvalid()) return btck_ValidationMode_INVALID;
    return btck_ValidationMode_INTERNAL_ERROR;
}

btck_BlockValidationResult btck_block_validation_state_get_block_validation_result(const btck_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = *cast_block_validation_state(block_validation_state_);
    switch (block_validation_state.GetResult()) {
    case BlockValidationResult::BLOCK_RESULT_UNSET:
        return btck_BlockValidationResult_UNSET;
    case BlockValidationResult::BLOCK_CONSENSUS:
        return btck_BlockValidationResult_CONSENSUS;
    case BlockValidationResult::BLOCK_CACHED_INVALID:
        return btck_BlockValidationResult_CACHED_INVALID;
    case BlockValidationResult::BLOCK_INVALID_HEADER:
        return btck_BlockValidationResult_INVALID_HEADER;
    case BlockValidationResult::BLOCK_MUTATED:
        return btck_BlockValidationResult_MUTATED;
    case BlockValidationResult::BLOCK_MISSING_PREV:
        return btck_BlockValidationResult_MISSING_PREV;
    case BlockValidationResult::BLOCK_INVALID_PREV:
        return btck_BlockValidationResult_INVALID_PREV;
    case BlockValidationResult::BLOCK_TIME_FUTURE:
        return btck_BlockValidationResult_TIME_FUTURE;
    case BlockValidationResult::BLOCK_HEADER_LOW_WORK:
        return btck_BlockValidationResult_HEADER_LOW_WORK;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

btck_ChainstateManagerOptions* btck_chainstate_manager_options_create(const btck_Context* context, const char* data_dir, size_t data_dir_len, const char* blocks_dir, size_t blocks_dir_len)
{
    try {
        fs::path abs_data_dir{fs::absolute(fs::PathFromString({data_dir, data_dir_len}))};
        fs::create_directories(abs_data_dir);
        fs::path abs_blocks_dir{fs::absolute(fs::PathFromString({blocks_dir, blocks_dir_len}))};
        fs::create_directories(abs_blocks_dir);
        auto chainman_opts{std::make_unique<ChainstateManagerOptions>(context->m_context, abs_data_dir, abs_blocks_dir)};
        return new btck_ChainstateManagerOptions{std::move(chainman_opts)};
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager options: %s", e.what());
        return nullptr;
    }
}

void btck_chainstate_manager_options_set_worker_threads_num(btck_ChainstateManagerOptions* opts, int worker_threads)
{
    LOCK(opts->m_opts->m_mutex);
    opts->m_opts->m_chainman_options.worker_threads_num = worker_threads;
}

void btck_chainstate_manager_options_destroy(btck_ChainstateManagerOptions* options)
{
    if (!options) return;
    delete options;
    options = nullptr;
}

int btck_chainstate_manager_options_set_wipe_dbs(btck_ChainstateManagerOptions* chainman_opts, int wipe_block_tree_db, int wipe_chainstate_db)
{
    if (wipe_block_tree_db == 1 && wipe_chainstate_db != 1) {
        LogError("Wiping the block tree db without also wiping the chainstate db is currently unsupported.");
        return -1;
    }
    LOCK(chainman_opts->m_opts->m_mutex);
    chainman_opts->m_opts->m_blockman_options.block_tree_db_params.wipe_data = wipe_block_tree_db == 1;
    chainman_opts->m_opts->m_chainstate_load_options.wipe_chainstate_db = wipe_chainstate_db == 1;
    return 0;
}

void btck_chainstate_manager_options_set_block_tree_db_in_memory(
    btck_ChainstateManagerOptions* chainman_opts,
    int block_tree_db_in_memory)
{
    LOCK(chainman_opts->m_opts->m_mutex);
    chainman_opts->m_opts->m_blockman_options.block_tree_db_params.memory_only = block_tree_db_in_memory == 1;
}

void btck_chainstate_manager_options_set_chainstate_db_in_memory(
    btck_ChainstateManagerOptions* chainman_opts,
    int chainstate_db_in_memory)
{
    LOCK(chainman_opts->m_opts->m_mutex);
    chainman_opts->m_opts->m_chainstate_load_options.coins_db_in_memory = chainstate_db_in_memory == 1;
}

btck_ChainstateManager* btck_chainstate_manager_create(
    const btck_ChainstateManagerOptions* chainman_opts)
{
    std::unique_ptr<ChainstateManager> chainman;
    try {
        LOCK(chainman_opts->m_opts->m_mutex);
        auto& context{chainman_opts->m_opts->m_context};
        chainman = std::make_unique<ChainstateManager>(*context->m_interrupt, chainman_opts->m_opts->m_chainman_options, chainman_opts->m_opts->m_blockman_options);
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager: %s", e.what());
        return nullptr;
    }

    try {
        const auto chainstate_load_opts{WITH_LOCK(chainman_opts->m_opts->m_mutex, return chainman_opts->m_opts->m_chainstate_load_options)};

        kernel::CacheSizes cache_sizes{DEFAULT_KERNEL_CACHE};
        auto [status, chainstate_err]{node::LoadChainstate(*chainman, cache_sizes, chainstate_load_opts)};
        if (status != node::ChainstateLoadStatus::SUCCESS) {
            LogError("Failed to load chain state from your data directory: %s", chainstate_err.original);
            return nullptr;
        }
        std::tie(status, chainstate_err) = node::VerifyLoadedChainstate(*chainman, chainstate_load_opts);
        if (status != node::ChainstateLoadStatus::SUCCESS) {
            LogError("Failed to verify loaded chain state from your datadir: %s", chainstate_err.original);
            return nullptr;
        }

        for (Chainstate* chainstate : WITH_LOCK(chainman->GetMutex(), return chainman->GetAll())) {
            BlockValidationState state;
            if (!chainstate->ActivateBestChain(state, nullptr)) {
                LogError("Failed to connect best block: %s", state.ToString());
                return nullptr;
            }
        }
    } catch (const std::exception& e) {
        LogError("Failed to load chainstate: %s", e.what());
        return nullptr;
    }

    return new btck_ChainstateManager{std::move(chainman), chainman_opts->m_opts->m_context};
}

btck_BlockTreeEntry* btck_chainstate_manager_get_block_tree_entry_by_hash(const btck_ChainstateManager* chainman, const btck_BlockHash* block_hash)
{
    auto hash = uint256{std::span<const unsigned char>{(*block_hash).hash, 32}};
    auto block_index = WITH_LOCK(chainman->m_chainman->GetMutex(), return chainman->m_chainman->m_blockman.LookupBlockIndex(hash));
    if (!block_index) {
        LogDebug(BCLog::KERNEL, "A block with the given hash is not indexed.");
        return nullptr;
    }
    return new btck_BlockTreeEntry{block_index};
}

void btck_chainstate_manager_destroy(btck_ChainstateManager* chainman)
{
    if (!chainman) return;

    {
        LOCK(chainman->m_chainman->GetMutex());
        for (Chainstate* chainstate : chainman->m_chainman->GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
                chainstate->ResetCoinsViews();
            }
        }
    }

    delete chainman;
    chainman = nullptr;
}

int btck_chainstate_manager_import_blocks(btck_ChainstateManager* chainman, const char** block_file_paths, size_t* block_file_paths_lens, size_t block_file_paths_len)
{
    try {
        std::vector<fs::path> import_files;
        import_files.reserve(block_file_paths_len);
        for (uint32_t i = 0; i < block_file_paths_len; i++) {
            if (block_file_paths[i] != nullptr) {
                import_files.emplace_back(std::string{block_file_paths[i], block_file_paths_lens[i]}.c_str());
            }
        }
        node::ImportBlocks(*chainman->m_chainman, import_files);
        chainman->m_chainman->ActiveChainstate().ForceFlushStateToDisk();
    } catch (const std::exception& e) {
        LogError("Failed to import blocks: %s", e.what());
        return -1;
    }
    return 0;
}

btck_Block* btck_block_create(const void* raw_block, size_t raw_block_length)
{
    auto block{std::make_shared<CBlock>()};

    DataStream stream{std::span{reinterpret_cast<const std::byte*>(raw_block), raw_block_length}};

    try {
        stream >> TX_WITH_WITNESS(*block);
    } catch (...) {
        LogDebug(BCLog::KERNEL, "Block decode failed.");
        return nullptr;
    }

    return new btck_Block{std::move(block)};
}

btck_Block* btck_block_copy(const btck_Block* block)
{
    return new btck_Block{block->m_block};
}

size_t btck_block_count_transactions(const btck_Block* block)
{
    return block->m_block->vtx.size();
}

btck_Transaction* btck_block_get_transaction_at(const btck_Block* block, size_t index)
{
    assert(index < block->m_block->vtx.size());
    return new btck_Transaction{block->m_block->vtx[index]};
}

int btck_block_to_bytes(const btck_Block* block, btck_WriteBytes writer, void* user_data)
{
    try {
        WriterStream ws{writer, user_data};
        ws << TX_WITH_WITNESS(*block->m_block);
        return 0;
    } catch (...) {
        return -1;
    }
}

btck_BlockHash* btck_block_get_hash(const btck_Block* block)
{
    auto hash{block->m_block->GetHash()};
    auto block_hash = new btck_BlockHash{};
    std::memcpy(block_hash->hash, hash.begin(), sizeof(hash));
    return block_hash;
}

void btck_block_destroy(btck_Block* block)
{
    if (!block) return;
    delete block;
    block = nullptr;
}

btck_Block* btck_block_read(const btck_ChainstateManager* chainman, const btck_BlockTreeEntry* entry)
{
    auto block{std::shared_ptr<CBlock>(new CBlock{})};
    if (!chainman->m_chainman->m_blockman.ReadBlock(*block, *entry->m_block_index)) {
        LogError("Failed to read block.");
        return nullptr;
    }
    return new btck_Block{block};
}

int32_t btck_block_tree_entry_get_height(const btck_BlockTreeEntry* entry)
{
    return entry->m_block_index->nHeight;
}

btck_BlockHash* btck_block_tree_entry_get_block_hash(const btck_BlockTreeEntry* entry)
{
    if (entry->m_block_index->phashBlock == nullptr) {
        return nullptr;
    }
    auto block_hash = new btck_BlockHash{};
    std::memcpy(block_hash->hash, entry->m_block_index->phashBlock->begin(), sizeof(*entry->m_block_index->phashBlock));
    return block_hash;
}

void btck_block_hash_destroy(btck_BlockHash* hash)
{
    if (hash) delete hash;
    hash = nullptr;
}

btck_BlockSpentOutputs* btck_block_spent_outputs_read(const btck_ChainstateManager* chainman, const btck_BlockTreeEntry* entry)
{
    if (entry->m_block_index->nHeight < 1) {
        LogDebug(BCLog::KERNEL, "The genesis block does not have any spent outputs.");
        return nullptr;
    }
    auto block_undo{std::make_shared<CBlockUndo>()};
    if (!chainman->m_chainman->m_blockman.ReadBlockUndo(*block_undo, *entry->m_block_index)) {
        LogError("Failed to read block spent outputs data.");
        return nullptr;
    }
    return new btck_BlockSpentOutputs{std::move(block_undo)};
}

btck_BlockSpentOutputs* btck_block_spent_outputs_copy(const btck_BlockSpentOutputs* block_spent_outputs)
{
    return new btck_BlockSpentOutputs{block_spent_outputs->m_block_undo};
}

size_t btck_block_spent_outputs_count(const btck_BlockSpentOutputs* block_spent_outputs)
{
    return block_spent_outputs->m_block_undo->vtxundo.size();
}

btck_TransactionSpentOutputs* btck_block_spent_outputs_get_transaction_spent_outputs_at(const btck_BlockSpentOutputs* block_spent_outputs, size_t transaction_index)
{
    assert(transaction_index < block_spent_outputs->m_block_undo->vtxundo.size());
    const auto* tx_undo{&block_spent_outputs->m_block_undo->vtxundo.at(transaction_index)};
    return new btck_TransactionSpentOutputs{tx_undo, false};
}

void btck_block_spent_outputs_destroy(btck_BlockSpentOutputs* block_spent_outputs)
{
    if (!block_spent_outputs) return;
    delete block_spent_outputs;
    block_spent_outputs = nullptr;
}

btck_TransactionSpentOutputs* btck_transaction_spent_outputs_copy(const btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    return new btck_TransactionSpentOutputs{new CTxUndo{*transaction_spent_outputs->m_tx_undo}, true};
}

size_t btck_transaction_spent_outputs_count(const btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    return transaction_spent_outputs->m_tx_undo->vprevout.size();
}

void btck_transaction_spent_outputs_destroy(btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    if (!transaction_spent_outputs) return;
    if (transaction_spent_outputs->m_owned) {
        delete transaction_spent_outputs->m_tx_undo;
    }
    delete transaction_spent_outputs;
    transaction_spent_outputs = nullptr;
}

btck_Coin* btck_transaction_spent_outputs_get_coin_at(const btck_TransactionSpentOutputs* transaction_spent_outputs, size_t coin_index)
{
    assert(coin_index < transaction_spent_outputs->m_tx_undo->vprevout.size());
    const Coin* coin{&transaction_spent_outputs->m_tx_undo->vprevout.at(coin_index)};
    return new btck_Coin{coin, false};
}

btck_Coin* btck_coin_copy(const btck_Coin* coin)
{
    return new btck_Coin{new Coin{*coin->m_coin}, true};
}

uint32_t btck_coin_confirmation_height(const btck_Coin* coin)
{
    return coin->m_coin->nHeight;
}

int btck_coin_is_coinbase(const btck_Coin* coin)
{
    return coin->m_coin->IsCoinBase() ? 1 : 0;
}

btck_TransactionOutput* btck_coin_get_output(const btck_Coin* coin)
{
    const CTxOut* output{&coin->m_coin->out};
    return new btck_TransactionOutput{output, false};
}

void btck_coin_destroy(btck_Coin* coin)
{
    if (!coin) return;
    if (coin->m_owned) {
        delete coin->m_coin;
    }
    delete coin;
    coin = nullptr;
}

int btck_chainstate_manager_process_block(
    btck_ChainstateManager* chainman,
    const btck_Block* block,
    int* _new_block)
{
    bool new_block;
    auto result = chainman->m_chainman->ProcessNewBlock(block->m_block, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
    if (_new_block) {
        *_new_block = new_block ? 1 : 0;
    }
    return result ? 0 : -1;
}

btck_Chain* btck_chainstate_manager_get_active_chain(const btck_ChainstateManager* chainman)
{
    return new btck_Chain{&WITH_LOCK(chainman->m_chainman->GetMutex(), return chainman->m_chainman->ActiveChain())};
}

btck_BlockTreeEntry* btck_chain_get_tip(const btck_Chain* chain)
{
    return new btck_BlockTreeEntry{chain->m_chain->Tip()};
}

btck_BlockTreeEntry* btck_chain_get_genesis(const btck_Chain* chain)
{
    return new btck_BlockTreeEntry{chain->m_chain->Genesis()};
}

btck_BlockTreeEntry* btck_chain_get_by_height(const btck_Chain* chain, int height)
{
    LOCK(::cs_main);
    assert(height >= 0 && height <= chain->m_chain->Height());
    return new btck_BlockTreeEntry{(*chain->m_chain)[height]};
}

int btck_chain_contains(const btck_Chain* chain, const btck_BlockTreeEntry* entry)
{
    LOCK(::cs_main);
    return chain->m_chain->Contains(entry->m_block_index) ? 1 : 0;
}

void btck_chain_destroy(btck_Chain* chain)
{
    // The chain is always unowned, so only delete the wrapper struct, not the data it is pointing to.
    delete chain;
}
