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

static const kernel::Context kernel_context_static{};

namespace {

/** Check that all specified flags are part of the libbitcoinkernel interface. */
bool verify_flags(unsigned int flags)
{
    return (flags & ~(kernel_SCRIPT_FLAGS_VERIFY_ALL)) == 0;
}

bool is_valid_flag_combination(unsigned int flags)
{
    if (flags & SCRIPT_VERIFY_CLEANSTACK && ~flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) return false;
    if (flags & SCRIPT_VERIFY_WITNESS && ~flags & SCRIPT_VERIFY_P2SH) return false;
    return true;
}

BCLog::Level get_bclog_level(const kernel_LogLevel level)
{
    switch (level) {
    case kernel_LogLevel::kernel_LOG_INFO: {
        return BCLog::Level::Info;
    }
    case kernel_LogLevel::kernel_LOG_DEBUG: {
        return BCLog::Level::Debug;
    }
    case kernel_LogLevel::kernel_LOG_TRACE: {
        return BCLog::Level::Trace;
    }
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

BCLog::LogFlags get_bclog_flag(const kernel_LogCategory category)
{
    switch (category) {
    case kernel_LogCategory::kernel_LOG_BENCH: {
        return BCLog::LogFlags::BENCH;
    }
    case kernel_LogCategory::kernel_LOG_BLOCKSTORAGE: {
        return BCLog::LogFlags::BLOCKSTORAGE;
    }
    case kernel_LogCategory::kernel_LOG_COINDB: {
        return BCLog::LogFlags::COINDB;
    }
    case kernel_LogCategory::kernel_LOG_LEVELDB: {
        return BCLog::LogFlags::LEVELDB;
    }
    case kernel_LogCategory::kernel_LOG_MEMPOOL: {
        return BCLog::LogFlags::MEMPOOL;
    }
    case kernel_LogCategory::kernel_LOG_PRUNE: {
        return BCLog::LogFlags::PRUNE;
    }
    case kernel_LogCategory::kernel_LOG_RAND: {
        return BCLog::LogFlags::RAND;
    }
    case kernel_LogCategory::kernel_LOG_REINDEX: {
        return BCLog::LogFlags::REINDEX;
    }
    case kernel_LogCategory::kernel_LOG_VALIDATION: {
        return BCLog::LogFlags::VALIDATION;
    }
    case kernel_LogCategory::kernel_LOG_KERNEL: {
        return BCLog::LogFlags::KERNEL;
    }
    case kernel_LogCategory::kernel_LOG_ALL: {
        return BCLog::LogFlags::ALL;
    }
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

kernel_SynchronizationState cast_state(SynchronizationState state)
{
    switch (state) {
    case SynchronizationState::INIT_REINDEX:
        return kernel_SynchronizationState::kernel_INIT_REINDEX;
    case SynchronizationState::INIT_DOWNLOAD:
        return kernel_SynchronizationState::kernel_INIT_DOWNLOAD;
    case SynchronizationState::POST_INIT:
        return kernel_SynchronizationState::kernel_POST_INIT;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

kernel_Warning cast_kernel_warning(kernel::Warning warning)
{
    switch (warning) {
    case kernel::Warning::UNKNOWN_NEW_RULES_ACTIVATED:
        return kernel_Warning::kernel_UNKNOWN_NEW_RULES_ACTIVATED;
    case kernel::Warning::LARGE_WORK_INVALID_CHAIN:
        return kernel_Warning::kernel_LARGE_WORK_INVALID_CHAIN;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

class KernelNotifications : public kernel::Notifications
{
private:
    kernel_NotificationInterfaceCallbacks m_cbs;

public:
    KernelNotifications(kernel_NotificationInterfaceCallbacks cbs)
        : m_cbs{cbs}
    {
    }

    kernel::InterruptResult blockTip(SynchronizationState state, CBlockIndex& index, double verification_progress) override
    {
        if (m_cbs.block_tip) m_cbs.block_tip((void*)m_cbs.user_data, cast_state(state), reinterpret_cast<const kernel_BlockIndex*>(&index), verification_progress);
        return {};
    }
    void headerTip(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) override
    {
        if (m_cbs.header_tip) m_cbs.header_tip((void*)m_cbs.user_data, cast_state(state), height, timestamp, presync);
    }
    void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override
    {
        if (m_cbs.progress) m_cbs.progress((void*)m_cbs.user_data, title.original.c_str(), title.original.length(), progress_percent, resume_possible);
    }
    void warningSet(kernel::Warning id, const bilingual_str& message) override
    {
        if (m_cbs.warning_set) m_cbs.warning_set((void*)m_cbs.user_data, cast_kernel_warning(id), message.original.c_str(), message.original.length());
    }
    void warningUnset(kernel::Warning id) override
    {
        if (m_cbs.warning_unset) m_cbs.warning_unset((void*)m_cbs.user_data, cast_kernel_warning(id));
    }
    void flushError(const bilingual_str& message) override
    {
        if (m_cbs.flush_error) m_cbs.flush_error((void*)m_cbs.user_data, message.original.c_str(), message.original.length());
    }
    void fatalError(const bilingual_str& message) override
    {
        if (m_cbs.fatal_error) m_cbs.fatal_error((void*)m_cbs.user_data, message.original.c_str(), message.original.length());
    }
};

class KernelValidationInterface final : public CValidationInterface
{
public:
    const kernel_ValidationInterfaceCallbacks m_cbs;

    explicit KernelValidationInterface(const kernel_ValidationInterfaceCallbacks vi_cbs) : m_cbs{vi_cbs} {}

protected:
    void BlockChecked(const CBlock& block, const BlockValidationState& stateIn) override
    {
        if (m_cbs.block_checked) {
            m_cbs.block_checked((void*)m_cbs.user_data,
                                reinterpret_cast<const kernel_BlockPointer*>(&block),
                                reinterpret_cast<const kernel_BlockValidationState*>(&stateIn));
        }
    }
};

struct ContextOptions {
    mutable Mutex m_mutex;
    std::unique_ptr<const CChainParams> m_chainparams GUARDED_BY(m_mutex);
    std::unique_ptr<const KernelNotifications> m_notifications GUARDED_BY(m_mutex);
    std::unique_ptr<const KernelValidationInterface> m_validation_interface GUARDED_BY(m_mutex);
};

class Context
{
public:
    std::unique_ptr<kernel::Context> m_context;

    std::unique_ptr<KernelNotifications> m_notifications;

    std::unique_ptr<util::SignalInterrupt> m_interrupt;

    std::unique_ptr<ValidationSignals> m_signals;

    std::unique_ptr<const CChainParams> m_chainparams;

    std::unique_ptr<KernelValidationInterface> m_validation_interface;

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
                m_notifications = std::make_unique<KernelNotifications>(*options->m_notifications);
            }
            if (options->m_validation_interface) {
                m_validation_interface = std::make_unique<KernelValidationInterface>(*options->m_validation_interface);
                m_signals->RegisterValidationInterface(m_validation_interface.get());
            }

        }

        if (!m_chainparams) {
            m_chainparams = CChainParams::Main();
        }
        if (!m_notifications) {
            m_notifications = std::make_unique<KernelNotifications>(kernel_NotificationInterfaceCallbacks{
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr});
        }

        if (!kernel::SanityChecks(*m_context)) {
            sane = false;
        }
    }

    ~Context()
    {
        m_signals->UnregisterValidationInterface(m_validation_interface.get());
    }
};

//! Helper struct to wrap the ChainstateManager-related Options
struct ChainstateManagerOptions {
    mutable Mutex m_mutex;
    ChainstateManager::Options m_chainman_options GUARDED_BY(m_mutex);
    node::BlockManager::Options m_blockman_options GUARDED_BY(m_mutex);
    node::ChainstateLoadOptions m_chainstate_load_options GUARDED_BY(m_mutex);

    ChainstateManagerOptions(const Context* context, const fs::path& data_dir, const fs::path& blocks_dir)
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
          m_chainstate_load_options{node::ChainstateLoadOptions{}}
    {
    }
};

const CTransaction* cast_transaction(const kernel_Transaction* transaction)
{
    assert(transaction);
    return reinterpret_cast<const CTransaction*>(transaction);
}

const CScript* cast_script_pubkey(const kernel_ScriptPubkey* script_pubkey)
{
    assert(script_pubkey);
    return reinterpret_cast<const CScript*>(script_pubkey);
}

const CTxOut* cast_transaction_output(const kernel_TransactionOutput* transaction_output)
{
    assert(transaction_output);
    return reinterpret_cast<const CTxOut*>(transaction_output);
}

const ContextOptions* cast_const_context_options(const kernel_ContextOptions* options)
{
    assert(options);
    return reinterpret_cast<const ContextOptions*>(options);
}

ContextOptions* cast_context_options(kernel_ContextOptions* options)
{
    assert(options);
    return reinterpret_cast<ContextOptions*>(options);
}

const CChainParams* cast_const_chain_params(const kernel_ChainParameters* chain_params)
{
    assert(chain_params);
    return reinterpret_cast<const CChainParams*>(chain_params);
}

CChainParams* cast_chain_params(kernel_ChainParameters* chain_params)
{
    assert(chain_params);
    return reinterpret_cast<CChainParams*>(chain_params);
}

Context* cast_context(kernel_Context* context)
{
    assert(context);
    return reinterpret_cast<Context*>(context);
}

const Context* cast_const_context(const kernel_Context* context)
{
    assert(context);
    return reinterpret_cast<const Context*>(context);
}

const ChainstateManagerOptions* cast_const_chainstate_manager_options(const kernel_ChainstateManagerOptions* options)
{
    assert(options);
    return reinterpret_cast<const ChainstateManagerOptions*>(options);
}

ChainstateManagerOptions* cast_chainstate_manager_options(kernel_ChainstateManagerOptions* options)
{
    assert(options);
    return reinterpret_cast<ChainstateManagerOptions*>(options);
}

ChainstateManager* cast_chainstate_manager(kernel_ChainstateManager* chainman)
{
    assert(chainman);
    return reinterpret_cast<ChainstateManager*>(chainman);
}

std::shared_ptr<CBlock>* cast_cblocksharedpointer(kernel_Block* block)
{
    assert(block);
    return reinterpret_cast<std::shared_ptr<CBlock>*>(block);
}

const BlockValidationState* cast_block_validation_state(const kernel_BlockValidationState* block_validation_state)
{
    assert(block_validation_state);
    return reinterpret_cast<const BlockValidationState*>(block_validation_state);
}

const CBlock* cast_const_cblock(const kernel_BlockPointer* block)
{
    assert(block);
    return reinterpret_cast<const CBlock*>(block);
}

const CBlockIndex* cast_const_block_index(const kernel_BlockIndex* index)
{
    assert(index);
    return reinterpret_cast<const CBlockIndex*>(index);
}

const CBlockUndo* cast_const_block_undo(const kernel_BlockUndo* undo)
{
    assert(undo);
    return reinterpret_cast<const CBlockUndo*>(undo);
}

CBlockUndo* cast_block_undo(kernel_BlockUndo* undo)
{
    assert(undo);
    return reinterpret_cast<CBlockUndo*>(undo);
}


} // namespace

kernel_Transaction* kernel_transaction_create(const unsigned char* raw_transaction, size_t raw_transaction_len)
{
    try {
        DataStream stream{std::span{raw_transaction, raw_transaction_len}};
        auto tx = new CTransaction{deserialize, TX_WITH_WITNESS, stream};
        return reinterpret_cast<kernel_Transaction*>(tx);
    } catch (const std::exception&) {
        return nullptr;
    }
}

void kernel_transaction_destroy(kernel_Transaction* transaction)
{
    if (transaction) {
        delete cast_transaction(transaction);
    }
}

kernel_ScriptPubkey* kernel_script_pubkey_create(const unsigned char* script_pubkey_, size_t script_pubkey_len)
{
    auto script_pubkey = new CScript(script_pubkey_, script_pubkey_ + script_pubkey_len);
    return reinterpret_cast<kernel_ScriptPubkey*>(script_pubkey);
}

kernel_ByteArray* kernel_script_pubkey_copy_data(const kernel_ScriptPubkey* script_pubkey_)
{
    auto script_pubkey{cast_script_pubkey(script_pubkey_)};

    auto byte_array{new kernel_ByteArray{
        .data = new unsigned char[script_pubkey->size()],
        .size = script_pubkey->size(),
    }};

    std::memcpy(byte_array->data, script_pubkey->data(), byte_array->size);
    return byte_array;
}

void kernel_script_pubkey_destroy(kernel_ScriptPubkey* script_pubkey)
{
    if (script_pubkey) {
        delete cast_script_pubkey(script_pubkey);
    }
}

kernel_TransactionOutput* kernel_transaction_output_create(const kernel_ScriptPubkey* script_pubkey_, int64_t amount)
{
    const auto& script_pubkey{*cast_script_pubkey(script_pubkey_)};
    const CAmount& value{amount};
    auto tx_out{new CTxOut(value, script_pubkey)};
    return reinterpret_cast<kernel_TransactionOutput*>(tx_out);
}

void kernel_transaction_output_destroy(kernel_TransactionOutput* output)
{
    if (output) {
        delete cast_transaction_output(output);
    }
}

bool kernel_verify_script(const kernel_ScriptPubkey* script_pubkey_,
                          const int64_t amount_,
                          const kernel_Transaction* tx_to,
                          const kernel_TransactionOutput** spent_outputs_, size_t spent_outputs_len,
                          const unsigned int input_index,
                          const unsigned int flags,
                          kernel_ScriptVerifyStatus* status)
{
    const CAmount amount{amount_};
    const auto& script_pubkey{*cast_script_pubkey(script_pubkey_)};

    if (!verify_flags(flags)) {
        if (status) *status = kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS;
        return false;
    }

    if (!is_valid_flag_combination(flags)) {
        if (status) *status = kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS_COMBINATION;
        return false;
    }

    if (flags & kernel_SCRIPT_FLAGS_VERIFY_TAPROOT && spent_outputs_ == nullptr) {
        if (status) *status = kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_REQUIRED;
        return false;
    }

    const CTransaction& tx{*cast_transaction(tx_to)};
    std::vector<CTxOut> spent_outputs;
    if (spent_outputs_ != nullptr) {
        if (spent_outputs_len != tx.vin.size()) {
            if (status) *status = kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_MISMATCH;
            return false;
        }
        spent_outputs.reserve(spent_outputs_len);
        for (size_t i = 0; i < spent_outputs_len; i++) {
            const CTxOut& tx_out{*reinterpret_cast<const CTxOut*>(spent_outputs_[i])};
            spent_outputs.push_back(tx_out);
        }
    }

    if (input_index >= tx.vin.size()) {
        if (status) *status = kernel_SCRIPT_VERIFY_ERROR_TX_INPUT_INDEX;
        return false;
    }
    PrecomputedTransactionData txdata{tx};

    if (spent_outputs_ != nullptr && flags & kernel_SCRIPT_FLAGS_VERIFY_TAPROOT) {
        txdata.Init(tx, std::move(spent_outputs));
    }

    return VerifyScript(tx.vin[input_index].scriptSig,
                        script_pubkey,
                        &tx.vin[input_index].scriptWitness,
                        flags,
                        TransactionSignatureChecker(&tx, input_index, amount, txdata, MissingDataBehavior::FAIL),
                        nullptr);
}

void kernel_logging_set_level_category(const kernel_LogCategory category, const kernel_LogLevel level)
{
    if (category == kernel_LogCategory::kernel_LOG_ALL) {
        LogInstance().SetLogLevel(get_bclog_level(level));
    }

    LogInstance().AddCategoryLogLevel(get_bclog_flag(category), get_bclog_level(level));
}

void kernel_logging_enable_category(const kernel_LogCategory category)
{
    LogInstance().EnableCategory(get_bclog_flag(category));
}

void kernel_logging_disable_category(const kernel_LogCategory category)
{
    LogInstance().DisableCategory(get_bclog_flag(category));
}

void kernel_logging_disable()
{
    LogInstance().DisableLogging();
}

kernel_LoggingConnection* kernel_logging_connection_create(kernel_LogCallback callback,
                                                           const void* user_data,
                                                           const kernel_LoggingOptions options)
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
            return nullptr;
        }
    } catch (std::exception&) {
        LogError("Logger start failed.");
        LogInstance().DeleteCallback(connection);
        return nullptr;
    }

    LogDebug(BCLog::KERNEL, "Logger connected.");

    auto heap_connection{new std::list<std::function<void(const std::string&)>>::iterator(connection)};
    return reinterpret_cast<kernel_LoggingConnection*>(heap_connection);
}

void kernel_logging_connection_destroy(kernel_LoggingConnection* connection_)
{
    auto connection{reinterpret_cast<std::list<std::function<void(const std::string&)>>::iterator*>(connection_)};
    if (!connection) {
        return;
    }

    LogDebug(BCLog::KERNEL, "Logger disconnected.");
    LogInstance().DeleteCallback(*connection);
    delete connection;

    // Switch back to buffering by calling DisconnectTestLogger if the
    // connection that was just removed was the last one.
    if (!LogInstance().Enabled()) {
        LogInstance().DisconnectTestLogger();
    }
}

kernel_ChainParameters* kernel_chain_parameters_create(const kernel_ChainType chain_type)
{
    switch (chain_type) {
    case kernel_ChainType::kernel_CHAIN_TYPE_MAINNET: {
        CChainParams* params = new CChainParams(*CChainParams::Main());
        return reinterpret_cast<kernel_ChainParameters*>(params);
    }
    case kernel_ChainType::kernel_CHAIN_TYPE_TESTNET: {
        CChainParams* params = new CChainParams(*CChainParams::TestNet());
        return reinterpret_cast<kernel_ChainParameters*>(params);
    }
    case kernel_ChainType::kernel_CHAIN_TYPE_TESTNET_4: {
        CChainParams* params = new CChainParams(*CChainParams::TestNet4());
        return reinterpret_cast<kernel_ChainParameters*>(params);
    }
    case kernel_ChainType::kernel_CHAIN_TYPE_SIGNET: {
        CChainParams* params = new CChainParams(*CChainParams::SigNet({}));
        return reinterpret_cast<kernel_ChainParameters*>(params);
    }
    case kernel_ChainType::kernel_CHAIN_TYPE_REGTEST: {
        CChainParams* params = new CChainParams(*CChainParams::RegTest({}));
        return reinterpret_cast<kernel_ChainParameters*>(params);
    }
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

void kernel_chain_parameters_destroy(kernel_ChainParameters* chain_parameters)
{
    if (chain_parameters) {
        delete cast_chain_params(chain_parameters);
    }
}

kernel_ContextOptions* kernel_context_options_create()
{
    return reinterpret_cast<kernel_ContextOptions*>(new ContextOptions{});
}

void kernel_context_options_set_chainparams(kernel_ContextOptions* options_, const kernel_ChainParameters* chain_parameters)
{
    auto options{cast_context_options(options_)};
    auto chain_params{cast_const_chain_params(chain_parameters)};
    // Copy the chainparams, so the caller can free it again
    LOCK(options->m_mutex);
    options->m_chainparams = std::make_unique<const CChainParams>(*chain_params);
}

void kernel_context_options_set_notifications(kernel_ContextOptions* options_, kernel_NotificationInterfaceCallbacks notifications)
{
    auto options{cast_context_options(options_)};
    // The KernelNotifications are copy-initialized, so the caller can free them again.
    LOCK(options->m_mutex);
    options->m_notifications = std::make_unique<const KernelNotifications>(notifications);
}

void kernel_context_options_set_validation_interface(kernel_ContextOptions* options_, kernel_ValidationInterfaceCallbacks vi_cbs)
{
    auto options{cast_context_options(options_)};
    LOCK(options->m_mutex);
    options->m_validation_interface = std::make_unique<KernelValidationInterface>(KernelValidationInterface(vi_cbs));
}

void kernel_context_options_destroy(kernel_ContextOptions* options)
{
    if (options) {
        delete cast_context_options(options);
    }
}

kernel_Context* kernel_context_create(const kernel_ContextOptions* options_)
{
    auto options{cast_const_context_options(options_)};
    bool sane{true};
    auto context{new Context{options, sane}};
    if (!sane) {
        LogError("Kernel context sanity check failed.");
        delete context;
        return nullptr;
    }
    return reinterpret_cast<kernel_Context*>(context);
}

bool kernel_context_interrupt(kernel_Context* context_)
{
    auto& context{*cast_context(context_)};
    return (*context.m_interrupt)();
}

void kernel_context_destroy(kernel_Context* context)
{
    if (context) {
        delete cast_context(context);
    }
}

kernel_ValidationMode kernel_block_validation_state_get_validation_mode(const kernel_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = *cast_block_validation_state(block_validation_state_);
    if (block_validation_state.IsValid()) return kernel_ValidationMode::kernel_VALIDATION_STATE_VALID;
    if (block_validation_state.IsInvalid()) return kernel_ValidationMode::kernel_VALIDATION_STATE_INVALID;
    return kernel_ValidationMode::kernel_VALIDATION_STATE_ERROR;
}

kernel_BlockValidationResult kernel_block_validation_state_get_block_validation_result(const kernel_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = *cast_block_validation_state(block_validation_state_);
    switch (block_validation_state.GetResult()) {
    case BlockValidationResult::BLOCK_RESULT_UNSET:
        return kernel_BlockValidationResult::kernel_BLOCK_RESULT_UNSET;
    case BlockValidationResult::BLOCK_CONSENSUS:
        return kernel_BlockValidationResult::kernel_BLOCK_CONSENSUS;
    case BlockValidationResult::BLOCK_CACHED_INVALID:
        return kernel_BlockValidationResult::kernel_BLOCK_CACHED_INVALID;
    case BlockValidationResult::BLOCK_INVALID_HEADER:
        return kernel_BlockValidationResult::kernel_BLOCK_INVALID_HEADER;
    case BlockValidationResult::BLOCK_MUTATED:
        return kernel_BlockValidationResult::kernel_BLOCK_MUTATED;
    case BlockValidationResult::BLOCK_MISSING_PREV:
        return kernel_BlockValidationResult::kernel_BLOCK_MISSING_PREV;
    case BlockValidationResult::BLOCK_INVALID_PREV:
        return kernel_BlockValidationResult::kernel_BLOCK_INVALID_PREV;
    case BlockValidationResult::BLOCK_TIME_FUTURE:
        return kernel_BlockValidationResult::kernel_BLOCK_TIME_FUTURE;
    case BlockValidationResult::BLOCK_HEADER_LOW_WORK:
        return kernel_BlockValidationResult::kernel_BLOCK_HEADER_LOW_WORK;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

kernel_ChainstateManagerOptions* kernel_chainstate_manager_options_create(const kernel_Context* context_, const char* data_dir, size_t data_dir_len, const char* blocks_dir, size_t blocks_dir_len)
{
    try {
        fs::path abs_data_dir{fs::absolute(fs::PathFromString({data_dir, data_dir_len}))};
        fs::create_directories(abs_data_dir);
        fs::path abs_blocks_dir{fs::absolute(fs::PathFromString({blocks_dir, blocks_dir_len}))};
        fs::create_directories(abs_blocks_dir);
        auto context{cast_const_context(context_)};
        return reinterpret_cast<kernel_ChainstateManagerOptions*>(new ChainstateManagerOptions(context, abs_data_dir, abs_blocks_dir));
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager options: %s", e.what());
        return nullptr;
    }
}

void kernel_chainstate_manager_options_set_worker_threads_num(kernel_ChainstateManagerOptions* opts_, int worker_threads)
{
    auto opts{cast_chainstate_manager_options(opts_)};
    LOCK(opts->m_mutex);
    opts->m_chainman_options.worker_threads_num = worker_threads;
}

void kernel_chainstate_manager_options_destroy(kernel_ChainstateManagerOptions* options)
{
    if (options) {
        delete cast_chainstate_manager_options(options);
    }
}

bool kernel_chainstate_manager_options_set_wipe_dbs(kernel_ChainstateManagerOptions* chainman_opts_, bool wipe_block_tree_db, bool wipe_chainstate_db)
{
    if (wipe_block_tree_db && !wipe_chainstate_db) {
        LogError("Wiping the block tree db without also wiping the chainstate db is currently unsupported.");
        return false;
    }
    auto opts{cast_chainstate_manager_options(chainman_opts_)};
    LOCK(opts->m_mutex);
    opts->m_blockman_options.block_tree_db_params.wipe_data = wipe_block_tree_db;
    opts->m_chainstate_load_options.wipe_chainstate_db = wipe_chainstate_db;
    return true;
}

void kernel_chainstate_manager_options_set_block_tree_db_in_memory(
    kernel_ChainstateManagerOptions* chainstate_load_opts_,
    bool block_tree_db_in_memory)
{
    auto opts{cast_chainstate_manager_options(chainstate_load_opts_)};
    LOCK(opts->m_mutex);
    opts->m_blockman_options.block_tree_db_params.memory_only = block_tree_db_in_memory;
}

void kernel_chainstate_manager_options_set_chainstate_db_in_memory(
    kernel_ChainstateManagerOptions* chainstate_load_opts_,
    bool chainstate_db_in_memory)
{
    auto opts{cast_chainstate_manager_options(chainstate_load_opts_)};
    LOCK(opts->m_mutex);
    opts->m_chainstate_load_options.coins_db_in_memory = chainstate_db_in_memory;
}

kernel_ChainstateManager* kernel_chainstate_manager_create(
    const kernel_Context* context_,
    const kernel_ChainstateManagerOptions* chainman_opts_)
{
    auto chainman_opts{cast_const_chainstate_manager_options(chainman_opts_)};
    auto context{cast_const_context(context_)};

    ChainstateManager* chainman;

    try {
        LOCK(chainman_opts->m_mutex);
        chainman = new ChainstateManager{*context->m_interrupt, chainman_opts->m_chainman_options, chainman_opts->m_blockman_options};
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager: %s", e.what());
        return nullptr;
    }

    try {
        const auto chainstate_load_opts{WITH_LOCK(chainman_opts->m_mutex, return chainman_opts->m_chainstate_load_options)};

        kernel::CacheSizes cache_sizes{DEFAULT_KERNEL_CACHE};
        auto [status, chainstate_err]{node::LoadChainstate(*chainman, cache_sizes, chainstate_load_opts)};
        if (status != node::ChainstateLoadStatus::SUCCESS) {
            LogError("Failed to load chain state from your data directory: %s", chainstate_err.original);
            kernel_chainstate_manager_destroy(reinterpret_cast<kernel_ChainstateManager*>(chainman), context_);
            return nullptr;
        }
        std::tie(status, chainstate_err) = node::VerifyLoadedChainstate(*chainman, chainstate_load_opts);
        if (status != node::ChainstateLoadStatus::SUCCESS) {
            LogError("Failed to verify loaded chain state from your datadir: %s", chainstate_err.original);
            kernel_chainstate_manager_destroy(reinterpret_cast<kernel_ChainstateManager*>(chainman), context_);
            return nullptr;
        }

        for (Chainstate* chainstate : WITH_LOCK(chainman->GetMutex(), return chainman->GetAll())) {
            BlockValidationState state;
            if (!chainstate->ActivateBestChain(state, nullptr)) {
                LogError("Failed to connect best block: %s", state.ToString());
                kernel_chainstate_manager_destroy(reinterpret_cast<kernel_ChainstateManager*>(chainman), context_);
                return nullptr;
            }
        }
    } catch (const std::exception& e) {
        LogError("Failed to load chainstate: %s", e.what());
        return nullptr;
    }

    return reinterpret_cast<kernel_ChainstateManager*>(chainman);
}

void kernel_chainstate_manager_destroy(kernel_ChainstateManager* chainman_, const kernel_Context* context_)
{
    if (!chainman_) return;

    auto chainman{cast_chainstate_manager(chainman_)};

    {
        LOCK(chainman->GetMutex());
        for (Chainstate* chainstate : chainman->GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
                chainstate->ResetCoinsViews();
            }
        }
    }

    delete chainman;
    return;
}

bool kernel_chainstate_manager_import_blocks(const kernel_Context* context_,
                          kernel_ChainstateManager* chainman_,
                          const char** block_file_paths,
                          size_t* block_file_paths_lens,
                          size_t block_file_paths_len)
{
    try {
        auto chainman{cast_chainstate_manager(chainman_)};
        std::vector<fs::path> import_files;
        import_files.reserve(block_file_paths_len);
        for (uint32_t i = 0; i < block_file_paths_len; i++) {
            if (block_file_paths[i] != nullptr) {
                import_files.emplace_back(std::string{block_file_paths[i], block_file_paths_lens[i]}.c_str());
            }
        }
        node::ImportBlocks(*chainman, import_files);
        chainman->ActiveChainstate().ForceFlushStateToDisk();
    } catch (const std::exception& e) {
        LogError("Failed to import blocks: %s", e.what());
        return false;
    }
    return true;
}

kernel_Block* kernel_block_create(const unsigned char* raw_block, size_t raw_block_length)
{
    auto block{new CBlock()};

    DataStream stream{std::span{raw_block, raw_block_length}};

    try {
        stream >> TX_WITH_WITNESS(*block);
    } catch (const std::exception&) {
        delete block;
        LogDebug(BCLog::KERNEL, "Block decode failed.");
        return nullptr;
    }

    return reinterpret_cast<kernel_Block*>(new std::shared_ptr<CBlock>(block));
}

void kernel_byte_array_destroy(kernel_ByteArray* byte_array)
{
    if (byte_array && byte_array->data) delete[] byte_array->data;
    if (byte_array) delete byte_array;
}

kernel_ByteArray* kernel_block_copy_data(kernel_Block* block_)
{
    auto block{cast_cblocksharedpointer(block_)};

    DataStream ss{};
    ss << TX_WITH_WITNESS(**block);

    auto byte_array{new kernel_ByteArray{
        .data = new unsigned char[ss.size()],
        .size = ss.size(),
    }};

    std::memcpy(byte_array->data, ss.data(), byte_array->size);

    return byte_array;
}

kernel_ByteArray* kernel_block_pointer_copy_data(const kernel_BlockPointer* block_)
{
    auto block{cast_const_cblock(block_)};

    DataStream ss{};
    ss << TX_WITH_WITNESS(*block);

    auto byte_array{new kernel_ByteArray{
        .data = new unsigned char[ss.size()],
        .size = ss.size(),
    }};

    std::memcpy(byte_array->data, ss.data(), byte_array->size);

    return byte_array;
}

kernel_BlockHash* kernel_block_get_hash(kernel_Block* block_)
{
    auto block{cast_cblocksharedpointer(block_)};
    auto hash{(*block)->GetHash()};
    auto block_hash = new kernel_BlockHash{};
    std::memcpy(block_hash->hash, hash.begin(), sizeof(hash));
    return block_hash;
}

kernel_BlockHash* kernel_block_pointer_get_hash(const kernel_BlockPointer* block_)
{
    auto block{cast_const_cblock(block_)};
    auto hash{block->GetHash()};
    auto block_hash = new kernel_BlockHash{};
    std::memcpy(block_hash->hash, hash.begin(), sizeof(hash));
    return block_hash;
}

void kernel_block_destroy(kernel_Block* block)
{
    if (block) {
        delete cast_cblocksharedpointer(block);
    }
}

kernel_BlockIndex* kernel_block_index_get_tip(const kernel_Context* context_, kernel_ChainstateManager* chainman_)
{
    auto chainman{cast_chainstate_manager(chainman_)};
    return reinterpret_cast<kernel_BlockIndex*>(WITH_LOCK(chainman->GetMutex(), return chainman->ActiveChain().Tip()));
}

kernel_BlockIndex* kernel_block_index_get_genesis(const kernel_Context* context_, kernel_ChainstateManager* chainman_)
{
    auto chainman{cast_chainstate_manager(chainman_)};
    return reinterpret_cast<kernel_BlockIndex*>(WITH_LOCK(chainman->GetMutex(), return chainman->ActiveChain().Genesis()));
}

kernel_BlockIndex* kernel_block_index_get_by_hash(const kernel_Context* context_, kernel_ChainstateManager* chainman_, kernel_BlockHash* block_hash)
{
    auto chainman{cast_chainstate_manager(chainman_)};

    auto hash = uint256{std::span<const unsigned char>{(*block_hash).hash, 32}};
    auto block_index = WITH_LOCK(chainman->GetMutex(), return chainman->m_blockman.LookupBlockIndex(hash));
    if (!block_index) {
        LogDebug(BCLog::KERNEL, "A block with the given hash is not indexed.");
        return nullptr;
    }
    return reinterpret_cast<kernel_BlockIndex*>(block_index);
}

kernel_BlockIndex* kernel_block_index_get_by_height(const kernel_Context* context_, kernel_ChainstateManager* chainman_, int height)
{
    auto chainman{cast_chainstate_manager(chainman_)};

    LOCK(chainman->GetMutex());

    if (height < 0 || height > chainman->ActiveChain().Height()) {
        LogDebug(BCLog::KERNEL, "Block height is out of range.");
        return nullptr;
    }
    return reinterpret_cast<kernel_BlockIndex*>(chainman->ActiveChain()[height]);
}

kernel_BlockIndex* kernel_block_index_get_next(const kernel_Context* context_, kernel_ChainstateManager* chainman_, const kernel_BlockIndex* block_index_)
{
    const auto block_index{cast_const_block_index(block_index_)};
    auto chainman{cast_chainstate_manager(chainman_)};

    auto next_block_index{WITH_LOCK(chainman->GetMutex(), return chainman->ActiveChain().Next(block_index))};

    if (!next_block_index) {
        LogTrace(BCLog::KERNEL, "The block index is the tip of the current chain, it does not have a next.");
    }

    return reinterpret_cast<kernel_BlockIndex*>(next_block_index);
}

kernel_BlockIndex* kernel_block_index_get_previous(const kernel_BlockIndex* block_index_)
{
    const CBlockIndex* block_index{cast_const_block_index(block_index_)};

    if (!block_index->pprev) {
        LogTrace(BCLog::KERNEL, "The block index is the genesis, it has no previous.");
        return nullptr;
    }

    return reinterpret_cast<kernel_BlockIndex*>(block_index->pprev);
}

kernel_Block* kernel_block_read(const kernel_Context* context_,
                                          kernel_ChainstateManager* chainman_,
                                          const kernel_BlockIndex* block_index_)
{
    auto chainman{cast_chainstate_manager(chainman_)};
    const CBlockIndex* block_index{cast_const_block_index(block_index_)};

    auto block{new std::shared_ptr<CBlock>(new CBlock{})};
    if (!chainman->m_blockman.ReadBlock(**block, *block_index)) {
        LogError("Failed to read block.");
        return nullptr;
    }
    return reinterpret_cast<kernel_Block*>(block);
}

kernel_BlockUndo* kernel_block_undo_read(const kernel_Context* context_,
                                                   kernel_ChainstateManager* chainman_,
                                                   const kernel_BlockIndex* block_index_)
{
    auto chainman{cast_chainstate_manager(chainman_)};
    const auto block_index{cast_const_block_index(block_index_)};

    if (block_index->nHeight < 1) {
        LogDebug(BCLog::KERNEL, "The genesis block does not have undo data.");
        return nullptr;
    }
    auto block_undo{new CBlockUndo{}};
    if (!chainman->m_blockman.ReadBlockUndo(*block_undo, *block_index)) {
        LogError("Failed to read block undo data.");
        return nullptr;
    }
    return reinterpret_cast<kernel_BlockUndo*>(block_undo);
}

void kernel_block_index_destroy(kernel_BlockIndex* block_index)
{
    // This is just a dummy function. The user does not control block index memory.
    return;
}

uint64_t kernel_block_undo_size(const kernel_BlockUndo* block_undo_)
{
    const auto block_undo{cast_const_block_undo(block_undo_)};
    return block_undo->vtxundo.size();
}

void kernel_block_undo_destroy(kernel_BlockUndo* block_undo)
{
    if (block_undo) {
        delete cast_block_undo(block_undo);
    }
}

uint64_t kernel_block_undo_get_transaction_undo_size(const kernel_BlockUndo* block_undo_, uint64_t transaction_undo_index)
{
    const auto block_undo{cast_const_block_undo(block_undo_)};
    if (transaction_undo_index >= block_undo->vtxundo.size()) {
        LogInfo("transaction undo index is out of bounds.");
        return 0;
    }
    return block_undo->vtxundo[transaction_undo_index].vprevout.size();
}

uint32_t kernel_block_undo_get_transaction_output_height_by_index(const kernel_BlockUndo* block_undo_, uint64_t transaction_undo_index, uint64_t output_index)
{
    const auto block_undo{cast_const_block_undo(block_undo_)};

    if (transaction_undo_index >= block_undo->vtxundo.size()) {
        LogInfo("transaction undo index is out of bounds.");
        return 0;
    }

    const auto& tx_undo = block_undo->vtxundo[transaction_undo_index];

    if (output_index >= tx_undo.vprevout.size()) {
        LogInfo("previous output index is out of bounds.");
        return 0;
    }

    return tx_undo.vprevout[output_index].nHeight;
}

kernel_TransactionOutput* kernel_block_undo_copy_transaction_output_by_index(const kernel_BlockUndo* block_undo_,
                                                          uint64_t transaction_undo_index,
                                                          uint64_t output_index)
{
    const auto block_undo{cast_const_block_undo(block_undo_)};

    if (transaction_undo_index >= block_undo->vtxundo.size()) {
        LogInfo("transaction undo index is out of bounds.");
        return nullptr;
    }

    const auto& tx_undo = block_undo->vtxundo[transaction_undo_index];

    if (output_index >= tx_undo.vprevout.size()) {
        LogInfo("previous output index is out of bounds.");
        return nullptr;
    }

    CTxOut* prevout{new CTxOut{tx_undo.vprevout.at(output_index).out}};
    return reinterpret_cast<kernel_TransactionOutput*>(prevout);
}

int32_t kernel_block_index_get_height(const kernel_BlockIndex* block_index_)
{
    auto block_index{cast_const_block_index(block_index_)};
    return block_index->nHeight;
}

kernel_BlockHash* kernel_block_index_get_block_hash(const kernel_BlockIndex* block_index_)
{
    auto block_index{cast_const_block_index(block_index_)};
    if (block_index->phashBlock == nullptr) {
        return nullptr;
    }
    auto block_hash = new kernel_BlockHash{};
    std::memcpy(block_hash->hash, block_index->phashBlock->begin(), sizeof(*block_index->phashBlock));
    return block_hash;
}

void kernel_block_hash_destroy(kernel_BlockHash* hash)
{
    if (hash) delete hash;
}

kernel_ScriptPubkey* kernel_transaction_output_copy_script_pubkey(kernel_TransactionOutput* output_)
{
    auto output{cast_transaction_output(output_)};
    auto script_pubkey = new CScript{output->scriptPubKey};
    return reinterpret_cast<kernel_ScriptPubkey*>(script_pubkey);
}

int64_t kernel_transaction_output_get_amount(kernel_TransactionOutput* output_)
{
    auto output{cast_transaction_output(output_)};
    return output->nValue;
}

bool kernel_chainstate_manager_process_block(
    const kernel_Context* context_,
    kernel_ChainstateManager* chainman_,
    kernel_Block* block_,
    bool* new_block)
{
    auto& chainman{*cast_chainstate_manager(chainman_)};

    auto blockptr{cast_cblocksharedpointer(block_)};

    return chainman.ProcessNewBlock(*blockptr, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/new_block);
}
