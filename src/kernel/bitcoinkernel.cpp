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
#include <kernel/cs_main.h>
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

namespace {

bool is_valid_flag_combination(script_verify_flags flags)
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

template <typename C, typename CPP>
struct Handle {
    static C* ref(CPP* cpp_type)
    {
        return reinterpret_cast<C*>(cpp_type);
    }

    static const C* ref(const CPP* cpp_type)
    {
        return reinterpret_cast<const C*>(cpp_type);
    }

    template <typename... Args>
    static C* create(Args&&... args)
    {
        auto cpp_obj{std::make_unique<CPP>(std::forward<Args>(args)...)};
        return reinterpret_cast<C*>(cpp_obj.release());
    }

    static C* copy(const C* ptr)
    {
        auto cpp_obj{std::make_unique<CPP>(get(ptr))};
        return reinterpret_cast<C*>(cpp_obj.release());
    }

    static const CPP& get(const C* ptr)
    {
        return *reinterpret_cast<const CPP*>(ptr);
    }

    static CPP& get(C* ptr)
    {
        return *reinterpret_cast<CPP*>(ptr);
    }

    static void operator delete(void* ptr)
    {
        delete reinterpret_cast<CPP*>(ptr);
    }
};

} // namespace

struct btck_BlockTreeEntry: Handle<btck_BlockTreeEntry, CBlockIndex> {};
struct btck_Block : Handle<btck_Block, std::shared_ptr<const CBlock>> {};
struct btck_BlockValidationState : Handle<btck_BlockValidationState, BlockValidationState> {};

namespace {

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

struct LoggingConnection {
    std::unique_ptr<std::list<std::function<void(const std::string&)>>::iterator> m_connection;
    void* m_user_data;
    std::function<void(void* user_data)> m_deleter;

    LoggingConnection(btck_LogCallback callback, void* user_data, btck_DestroyCallback user_data_destroy_callback, const btck_LoggingOptions options)
    {
        LOCK(cs_main);

        LogInstance().m_log_timestamps = options.log_timestamps;
        LogInstance().m_log_time_micros = options.log_time_micros;
        LogInstance().m_log_threadnames = options.log_threadnames;
        LogInstance().m_log_sourcelocations = options.log_sourcelocations;
        LogInstance().m_always_print_category_level = options.always_print_category_levels;

        auto connection{LogInstance().PushBackCallback([callback, user_data](const std::string& str) { callback(user_data, str.c_str(), str.length()); })};

        try {
            // Only start logging if we just added the connection.
            if (LogInstance().NumConnections() == 1 && !LogInstance().StartLogging()) {
                LogError("Logger start failed.");
                LogInstance().DeleteCallback(connection);
                user_data_destroy_callback(user_data);
            }
        } catch (std::exception& e) {
            LogError("Logger start failed: %s", e.what());
            LogInstance().DeleteCallback(connection);
            user_data_destroy_callback(user_data);
            throw;
        }

        m_connection = std::make_unique<std::list<std::function<void(const std::string&)>>::iterator>(connection);
        m_user_data = user_data;
        m_deleter = user_data_destroy_callback;

        LogDebug(BCLog::KERNEL, "Logger connected.");
    }

    ~LoggingConnection()
    {
        LOCK(cs_main);

        LogDebug(BCLog::KERNEL, "Logger disconnected.");
        LogInstance().DeleteCallback(*m_connection);
        m_connection.reset();
        if (m_user_data && m_deleter) {
            m_deleter(m_user_data);
        }

        // Switch back to buffering by calling DisconnectTestLogger if the
        // connection that was just removed was the last one.
        if (!LogInstance().Enabled()) {
            LogInstance().DisconnectTestLogger();
        }
    }
};

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

    kernel::InterruptResult blockTip(SynchronizationState state, const CBlockIndex& index, double verification_progress) override
    {
        if (m_cbs.block_tip) m_cbs.block_tip(m_cbs.user_data, cast_state(state), btck_BlockTreeEntry::ref(&index), verification_progress);
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
            m_cbs.block_checked(m_cbs.user_data,
                                btck_Block::ref(new std::shared_ptr<const CBlock>{block}),
                                btck_BlockValidationState::ref(&stateIn));
        }
    }

    void NewPoWValidBlock(const CBlockIndex* pindex, const std::shared_ptr<const CBlock>& block) override
    {
        if (m_cbs.pow_valid_block) {
            m_cbs.pow_valid_block(m_cbs.user_data,
                                  btck_BlockTreeEntry::ref(pindex),
                                  btck_Block::ref(new std::shared_ptr<const CBlock>{block}));
        }
    }

    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        if (m_cbs.block_connected) {
            m_cbs.block_connected(m_cbs.user_data,
                                  btck_Block::ref(new std::shared_ptr<const CBlock>{block}),
                                  btck_BlockTreeEntry::ref(pindex));
        }
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        if (m_cbs.block_disconnected) {
            m_cbs.block_disconnected(m_cbs.user_data,
                                     btck_Block::ref(new std::shared_ptr<const CBlock>{block}),
                                     btck_BlockTreeEntry::ref(pindex));
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
    std::shared_ptr<const Context> m_context;
    node::ChainstateLoadOptions m_chainstate_load_options GUARDED_BY(m_mutex);

    ChainstateManagerOptions(const std::shared_ptr<const Context>& context, const fs::path& data_dir, const fs::path& blocks_dir)
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
          m_context{context}, m_chainstate_load_options{node::ChainstateLoadOptions{}}
    {
    }
};

struct ChainMan {
    std::unique_ptr<ChainstateManager> m_chainman;
    std::shared_ptr<const Context> m_context;

    ChainMan(std::unique_ptr<ChainstateManager> chainman, std::shared_ptr<const Context> context)
        : m_chainman(std::move(chainman)), m_context(std::move(context)) {}
};

} // namespace

struct btck_Transaction : Handle<btck_Transaction, std::shared_ptr<const CTransaction>> {};
struct btck_TransactionOutput : Handle<btck_TransactionOutput, CTxOut> {};
struct btck_ScriptPubkey : Handle<btck_ScriptPubkey, CScript> {};
struct btck_LoggingConnection : Handle<btck_LoggingConnection, LoggingConnection> {};
struct btck_ContextOptions : Handle<btck_ContextOptions, ContextOptions> {};
struct btck_Context : Handle<btck_Context, std::shared_ptr<const Context>> {};
struct btck_ChainParameters : Handle<btck_ChainParameters, std::unique_ptr<const CChainParams>> {};
struct btck_ChainstateManagerOptions : Handle<btck_ChainstateManagerOptions, ChainstateManagerOptions> {};
struct btck_ChainstateManager : Handle<btck_ChainstateManager, ChainMan> {};
struct btck_Chain : Handle<btck_Chain, CChain> {};
struct btck_BlockSpentOutputs : Handle<btck_BlockSpentOutputs, std::shared_ptr<CBlockUndo>> {};
struct btck_TransactionSpentOutputs : Handle<btck_TransactionSpentOutputs, CTxUndo> {};
struct btck_Coin : Handle<btck_Coin, Coin> {};
struct btck_BlockHash : Handle<btck_BlockHash, uint256> {};
struct btck_TransactionInput : Handle<btck_TransactionInput, CTxIn> {};
struct btck_TransactionOutPoint: Handle<btck_TransactionOutPoint, COutPoint> {};
struct btck_Txid: Handle<btck_Txid, Txid> {};

btck_Transaction* btck_transaction_create(const void* raw_transaction, size_t raw_transaction_len)
{
    try {
        DataStream stream{std::span{reinterpret_cast<const std::byte*>(raw_transaction), raw_transaction_len}};
        return btck_Transaction::create(std::make_shared<const CTransaction>(deserialize, TX_WITH_WITNESS, stream));
    } catch (...) {
        return nullptr;
    }
}

size_t btck_transaction_count_outputs(const btck_Transaction* transaction)
{
    return btck_Transaction::get(transaction)->vout.size();
}

const btck_TransactionOutput* btck_transaction_get_output_at(const btck_Transaction* transaction, size_t output_index)
{
    const CTransaction& tx = *btck_Transaction::get(transaction);
    assert(output_index < tx.vout.size());
    return btck_TransactionOutput::ref(&tx.vout[output_index]);
}

size_t btck_transaction_count_inputs(const btck_Transaction* transaction)
{
    return btck_Transaction::get(transaction)->vin.size();
}

const btck_TransactionInput* btck_transaction_get_input_at(const btck_Transaction* transaction, size_t input_index)
{
    assert(input_index < btck_Transaction::get(transaction)->vin.size());
    return btck_TransactionInput::ref(&btck_Transaction::get(transaction)->vin[input_index]);
}

const btck_Txid* btck_transaction_get_txid(const btck_Transaction* transaction)
{
    return btck_Txid::ref(&btck_Transaction::get(transaction)->GetHash());
}

btck_Transaction* btck_transaction_copy(const btck_Transaction* transaction)
{
    return btck_Transaction::copy(transaction);
}

int btck_transaction_to_bytes(const btck_Transaction* transaction, btck_WriteBytes writer, void* user_data)
{
    try {
        WriterStream ws{writer, user_data};
        ws << TX_WITH_WITNESS(btck_Transaction::get(transaction));
        return 0;
    } catch (...) {
        return -1;
    }
}

void btck_transaction_destroy(btck_Transaction* transaction)
{
    delete transaction;
}

btck_ScriptPubkey* btck_script_pubkey_create(const void* script_pubkey, size_t script_pubkey_len)
{
    auto data = std::span{reinterpret_cast<const uint8_t*>(script_pubkey), script_pubkey_len};
    return btck_ScriptPubkey::create(data.begin(), data.end());
}

int btck_script_pubkey_to_bytes(const btck_ScriptPubkey* script_pubkey_, btck_WriteBytes writer, void* user_data)
{
    const auto& script_pubkey{btck_ScriptPubkey::get(script_pubkey_)};
    return writer(script_pubkey.data(), script_pubkey.size(), user_data);
}

btck_ScriptPubkey* btck_script_pubkey_copy(const btck_ScriptPubkey* script_pubkey)
{
    return btck_ScriptPubkey::copy(script_pubkey);
}

void btck_script_pubkey_destroy(btck_ScriptPubkey* script_pubkey)
{
    delete script_pubkey;
}

btck_TransactionOutput* btck_transaction_output_create(const btck_ScriptPubkey* script_pubkey, int64_t amount)
{
    return btck_TransactionOutput::create(amount, btck_ScriptPubkey::get(script_pubkey));
}

btck_TransactionOutput* btck_transaction_output_copy(const btck_TransactionOutput* output)
{
    return btck_TransactionOutput::copy(output);
}

const btck_ScriptPubkey* btck_transaction_output_get_script_pubkey(const btck_TransactionOutput* output)
{
    return btck_ScriptPubkey::ref(&btck_TransactionOutput::get(output).scriptPubKey);
}

int64_t btck_transaction_output_get_amount(const btck_TransactionOutput* output)
{
    return btck_TransactionOutput::get(output).nValue;
}

void btck_transaction_output_destroy(btck_TransactionOutput* output)
{
    delete output;
}

int btck_script_pubkey_verify(const btck_ScriptPubkey* script_pubkey,
                              const int64_t amount,
                              const btck_Transaction* tx_to,
                              const btck_TransactionOutput** spent_outputs_, size_t spent_outputs_len,
                              const unsigned int input_index,
                              const btck_ScriptVerificationFlags flags,
                              btck_ScriptVerifyStatus* status)
{
    // Assert that all specified flags are part of the interface before continuing
    assert((flags & ~btck_ScriptVerificationFlags_ALL) == 0);

    if (!is_valid_flag_combination(script_verify_flags::from_int(flags))) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION;
        return 0;
    }

    if (flags & btck_ScriptVerificationFlags_TAPROOT && spent_outputs_ == nullptr) {
        if (status) *status = btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED;
        return 0;
    }

    const CTransaction& tx{*btck_Transaction::get(tx_to)};
    std::vector<CTxOut> spent_outputs;
    if (spent_outputs_ != nullptr) {
        assert(spent_outputs_len == tx.vin.size());
        spent_outputs.reserve(spent_outputs_len);
        for (size_t i = 0; i < spent_outputs_len; i++) {
            const CTxOut& tx_out{btck_TransactionOutput::get(spent_outputs_[i])};
            spent_outputs.push_back(tx_out);
        }
    }

    assert(input_index < tx.vin.size());
    PrecomputedTransactionData txdata{tx};

    if (spent_outputs_ != nullptr && flags & btck_ScriptVerificationFlags_TAPROOT) {
        txdata.Init(tx, std::move(spent_outputs));
    }

    bool result = VerifyScript(tx.vin[input_index].scriptSig,
                               btck_ScriptPubkey::get(script_pubkey),
                               &tx.vin[input_index].scriptWitness,
                               script_verify_flags::from_int(flags),
                               TransactionSignatureChecker(&tx, input_index, amount, txdata, MissingDataBehavior::FAIL),
                               nullptr);
    return result ? 1 : 0;
}

btck_TransactionInput* btck_transaction_input_copy(const btck_TransactionInput* input)
{
    return btck_TransactionInput::copy(input);
}

const btck_TransactionOutPoint* btck_transaction_input_get_out_point(const btck_TransactionInput* input)
{
    return btck_TransactionOutPoint::ref(&btck_TransactionInput::get(input).prevout);
}

void btck_transaction_input_destroy(btck_TransactionInput* input)
{
    delete input;
}

btck_TransactionOutPoint* btck_transaction_out_point_copy(const btck_TransactionOutPoint* out_point)
{
    return btck_TransactionOutPoint::copy(out_point);
}

uint32_t btck_transaction_out_point_get_index(const btck_TransactionOutPoint* out_point)
{
    return btck_TransactionOutPoint::get(out_point).n;
}

const btck_Txid* btck_transaction_out_point_get_txid(const btck_TransactionOutPoint* out_point)
{
    return btck_Txid::ref(&btck_TransactionOutPoint::get(out_point).hash);
}

void btck_transaction_out_point_destroy(btck_TransactionOutPoint* out_point)
{
    delete out_point;
}

btck_Txid* btck_txid_copy(const btck_Txid* txid)
{
    return btck_Txid::copy(txid);
}

void btck_txid_to_bytes(const btck_Txid* txid, unsigned char output[32])
{
    std::memcpy(output, btck_Txid::get(txid).begin(), 32);
}

int btck_txid_equals(const btck_Txid* txid1, const btck_Txid* txid2)
{
    return btck_Txid::get(txid1) == btck_Txid::get(txid2);
}

void btck_txid_destroy(btck_Txid* txid)
{
    delete txid;
}

void btck_logging_set_level_category(btck_LogCategory category, btck_LogLevel level)
{
    LOCK(cs_main);
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
    return btck_LoggingConnection::create(callback, user_data, user_data_destroy_callback, options);
}

void btck_logging_connection_destroy(btck_LoggingConnection* connection)
{
    delete connection;
}

btck_ChainParameters* btck_chain_parameters_create(const btck_ChainType chain_type)
{
    switch (chain_type) {
    case btck_ChainType_MAINNET: {
        return btck_ChainParameters::create(CChainParams::Main());
    }
    case btck_ChainType_TESTNET: {
        return btck_ChainParameters::create(CChainParams::TestNet());
    }
    case btck_ChainType_TESTNET_4: {
        return btck_ChainParameters::create(CChainParams::TestNet4());
    }
    case btck_ChainType_SIGNET: {
        return btck_ChainParameters::create(CChainParams::SigNet({}));
    }
    case btck_ChainType_REGTEST: {
        return btck_ChainParameters::create(CChainParams::RegTest({}));
    }
    }
    assert(false);
}

btck_ChainParameters* btck_chain_parameters_copy(const btck_ChainParameters* chain_parameters)
{
    const auto& original = btck_ChainParameters::get(chain_parameters);
    return btck_ChainParameters::create(std::make_unique<const CChainParams>(*original));
}

void btck_chain_parameters_destroy(btck_ChainParameters* chain_parameters)
{
    delete chain_parameters;
}

btck_ContextOptions* btck_context_options_create()
{
    return btck_ContextOptions::create();
}

void btck_context_options_set_chainparams(btck_ContextOptions* options, const btck_ChainParameters* chain_parameters)
{
    // Copy the chainparams, so the caller can free it again
    LOCK(btck_ContextOptions::get(options).m_mutex);
    btck_ContextOptions::get(options).m_chainparams = std::make_unique<const CChainParams>(*btck_ChainParameters::get(chain_parameters));
}

void btck_context_options_set_notifications(btck_ContextOptions* options, btck_NotificationInterfaceCallbacks notifications)
{
    // The KernelNotifications are copy-initialized, so the caller can free them again.
    LOCK(btck_ContextOptions::get(options).m_mutex);
    btck_ContextOptions::get(options).m_notifications = std::make_shared<KernelNotifications>(notifications);
}

void btck_context_options_set_validation_interface(btck_ContextOptions* options, btck_ValidationInterfaceCallbacks vi_cbs)
{
    LOCK(btck_ContextOptions::get(options).m_mutex);
    btck_ContextOptions::get(options).m_validation_interface = std::make_shared<KernelValidationInterface>(vi_cbs);
}

void btck_context_options_destroy(btck_ContextOptions* options)
{
    delete options;
}

btck_Context* btck_context_create(const btck_ContextOptions* options)
{
    bool sane{true};
    const ContextOptions* opts = options ? &btck_ContextOptions::get(options) : nullptr;
    auto context{std::make_shared<const Context>(opts, sane)};
    if (!sane) {
        LogError("Kernel context sanity check failed.");
        return nullptr;
    }
    return btck_Context::create(context);
}

btck_Context* btck_context_copy(const btck_Context* context)
{
    return btck_Context::copy(context);
}

int btck_context_interrupt(btck_Context* context)
{
    return (*btck_Context::get(context)->m_interrupt)() ? 0 : -1;
}

void btck_context_destroy(btck_Context* context)
{
    delete context;
}

const btck_BlockTreeEntry* btck_block_tree_entry_get_previous(const btck_BlockTreeEntry* entry)
{
    if (!btck_BlockTreeEntry::get(entry).pprev) {
        LogInfo("Genesis block has no previous.");
        return nullptr;
    }

    return btck_BlockTreeEntry::ref(btck_BlockTreeEntry::get(entry).pprev);
}

btck_ValidationMode btck_block_validation_state_get_validation_mode(const btck_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = btck_BlockValidationState::get(block_validation_state_);
    if (block_validation_state.IsValid()) return btck_ValidationMode_VALID;
    if (block_validation_state.IsInvalid()) return btck_ValidationMode_INVALID;
    return btck_ValidationMode_INTERNAL_ERROR;
}

btck_BlockValidationResult btck_block_validation_state_get_block_validation_result(const btck_BlockValidationState* block_validation_state_)
{
    auto& block_validation_state = btck_BlockValidationState::get(block_validation_state_);
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
        return btck_ChainstateManagerOptions::create(btck_Context::get(context), abs_data_dir, abs_blocks_dir);
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager options: %s", e.what());
        return nullptr;
    }
}

void btck_chainstate_manager_options_set_worker_threads_num(btck_ChainstateManagerOptions* opts, int worker_threads)
{
    LOCK(btck_ChainstateManagerOptions::get(opts).m_mutex);
    btck_ChainstateManagerOptions::get(opts).m_chainman_options.worker_threads_num = worker_threads;
}

void btck_chainstate_manager_options_destroy(btck_ChainstateManagerOptions* options)
{
    delete options;
}

int btck_chainstate_manager_options_set_wipe_dbs(btck_ChainstateManagerOptions* chainman_opts, int wipe_block_tree_db, int wipe_chainstate_db)
{
    if (wipe_block_tree_db == 1 && wipe_chainstate_db != 1) {
        LogError("Wiping the block tree db without also wiping the chainstate db is currently unsupported.");
        return -1;
    }
    auto& opts{btck_ChainstateManagerOptions::get(chainman_opts)};
    LOCK(opts.m_mutex);
    opts.m_blockman_options.block_tree_db_params.wipe_data = wipe_block_tree_db == 1;
    opts.m_chainstate_load_options.wipe_chainstate_db = wipe_chainstate_db == 1;
    return 0;
}

void btck_chainstate_manager_options_update_block_tree_db_in_memory(
    btck_ChainstateManagerOptions* chainman_opts,
    int block_tree_db_in_memory)
{
    auto& opts{btck_ChainstateManagerOptions::get(chainman_opts)};
    LOCK(opts.m_mutex);
    opts.m_blockman_options.block_tree_db_params.memory_only = block_tree_db_in_memory == 1;
}

void btck_chainstate_manager_options_update_chainstate_db_in_memory(
    btck_ChainstateManagerOptions* chainman_opts,
    int chainstate_db_in_memory)
{
    auto& opts{btck_ChainstateManagerOptions::get(chainman_opts)};
    LOCK(opts.m_mutex);
    opts.m_chainstate_load_options.coins_db_in_memory = chainstate_db_in_memory == 1;
}

btck_ChainstateManager* btck_chainstate_manager_create(
    const btck_ChainstateManagerOptions* chainman_opts)
{
    auto& opts{btck_ChainstateManagerOptions::get(chainman_opts)};
    std::unique_ptr<ChainstateManager> chainman;
    try {
        LOCK(opts.m_mutex);
        chainman = std::make_unique<ChainstateManager>(*opts.m_context->m_interrupt, opts.m_chainman_options, opts.m_blockman_options);
    } catch (const std::exception& e) {
        LogError("Failed to create chainstate manager: %s", e.what());
        return nullptr;
    }

    try {
        const auto chainstate_load_opts{WITH_LOCK(opts.m_mutex, return opts.m_chainstate_load_options)};

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

    return btck_ChainstateManager::create(std::move(chainman), opts.m_context);
}

const btck_BlockTreeEntry* btck_chainstate_manager_get_block_tree_entry_by_hash(const btck_ChainstateManager* chainman, const btck_BlockHash* block_hash)
{
    auto block_index = WITH_LOCK(btck_ChainstateManager::get(chainman).m_chainman->GetMutex(),
                                 return btck_ChainstateManager::get(chainman).m_chainman->m_blockman.LookupBlockIndex(btck_BlockHash::get(block_hash)));
    if (!block_index) {
        LogDebug(BCLog::KERNEL, "A block with the given hash is not indexed.");
        return nullptr;
    }
    return btck_BlockTreeEntry::ref(block_index);
}

void btck_chainstate_manager_destroy(btck_ChainstateManager* chainman)
{
    {
        LOCK(btck_ChainstateManager::get(chainman).m_chainman->GetMutex());
        for (Chainstate* chainstate : btck_ChainstateManager::get(chainman).m_chainman->GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
                chainstate->ResetCoinsViews();
            }
        }
    }

    delete chainman;
}

int btck_chainstate_manager_import_blocks(btck_ChainstateManager* chainman, const char** block_file_paths_data, size_t* block_file_paths_lens, size_t block_file_paths_data_len)
{
    try {
        std::vector<fs::path> import_files;
        import_files.reserve(block_file_paths_data_len);
        for (uint32_t i = 0; i < block_file_paths_data_len; i++) {
            if (block_file_paths_data[i] != nullptr) {
                import_files.emplace_back(std::string{block_file_paths_data[i], block_file_paths_lens[i]}.c_str());
            }
        }
        node::ImportBlocks(*btck_ChainstateManager::get(chainman).m_chainman, import_files);
        btck_ChainstateManager::get(chainman).m_chainman->ActiveChainstate().ForceFlushStateToDisk();
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

    return btck_Block::create(block);
}

btck_Block* btck_block_copy(const btck_Block* block)
{
    return btck_Block::copy(block);
}

size_t btck_block_count_transactions(const btck_Block* block)
{
    return btck_Block::get(block)->vtx.size();
}

const btck_Transaction* btck_block_get_transaction_at(const btck_Block* block, size_t index)
{
    assert(index < btck_Block::get(block)->vtx.size());
    return btck_Transaction::ref(&btck_Block::get(block)->vtx[index]);
}

int btck_block_to_bytes(const btck_Block* block, btck_WriteBytes writer, void* user_data)
{
    try {
        WriterStream ws{writer, user_data};
        ws << TX_WITH_WITNESS(*btck_Block::get(block));
        return 0;
    } catch (...) {
        return -1;
    }
}

btck_BlockHash* btck_block_get_hash(const btck_Block* block)
{
    return btck_BlockHash::create(btck_Block::get(block)->GetHash());
}

void btck_block_destroy(btck_Block* block)
{
    delete block;
}

btck_Block* btck_block_read(const btck_ChainstateManager* chainman, const btck_BlockTreeEntry* entry)
{
    auto block{std::make_shared<CBlock>()};
    if (!btck_ChainstateManager::get(chainman).m_chainman->m_blockman.ReadBlock(*block, btck_BlockTreeEntry::get(entry))) {
        LogError("Failed to read block.");
        return nullptr;
    }
    return btck_Block::create(block);
}

int32_t btck_block_tree_entry_get_height(const btck_BlockTreeEntry* entry)
{
    return btck_BlockTreeEntry::get(entry).nHeight;
}

btck_BlockHash* btck_block_tree_entry_get_block_hash(const btck_BlockTreeEntry* entry)
{
    if (btck_BlockTreeEntry::get(entry).phashBlock == nullptr) {
        return nullptr;
    }
    return btck_BlockHash::create(btck_BlockTreeEntry::get(entry).GetBlockHash());
}

btck_BlockHash* btck_block_hash_create(const unsigned char block_hash[32])
{
    return btck_BlockHash::create(std::span<const unsigned char>{block_hash, 32});
}

btck_BlockHash* btck_block_hash_copy(const btck_BlockHash* block_hash)
{
    return btck_BlockHash::copy(block_hash);
}

void btck_block_hash_to_bytes(const btck_BlockHash* block_hash, unsigned char output[32])
{
    std::memcpy(output, btck_BlockHash::get(block_hash).begin(), 32);
}

int btck_block_hash_equals(const btck_BlockHash* hash1, const btck_BlockHash* hash2)
{
    return btck_BlockHash::get(hash1) == btck_BlockHash::get(hash2);
}

void btck_block_hash_destroy(btck_BlockHash* hash)
{
    delete hash;
}

btck_BlockSpentOutputs* btck_block_spent_outputs_read(const btck_ChainstateManager* chainman, const btck_BlockTreeEntry* entry)
{
    auto block_undo{std::make_shared<CBlockUndo>()};
    if (btck_BlockTreeEntry::get(entry).nHeight < 1) {
        LogDebug(BCLog::KERNEL, "The genesis block does not have any spent outputs.");
        return btck_BlockSpentOutputs::create(block_undo);
    }
    if (!btck_ChainstateManager::get(chainman).m_chainman->m_blockman.ReadBlockUndo(*block_undo, btck_BlockTreeEntry::get(entry))) {
        LogError("Failed to read block spent outputs data.");
        return nullptr;
    }
    return btck_BlockSpentOutputs::create(block_undo);
}

btck_BlockSpentOutputs* btck_block_spent_outputs_copy(const btck_BlockSpentOutputs* block_spent_outputs)
{
    return btck_BlockSpentOutputs::copy(block_spent_outputs);
}

size_t btck_block_spent_outputs_count(const btck_BlockSpentOutputs* block_spent_outputs)
{
    return btck_BlockSpentOutputs::get(block_spent_outputs)->vtxundo.size();
}

const btck_TransactionSpentOutputs* btck_block_spent_outputs_get_transaction_spent_outputs_at(const btck_BlockSpentOutputs* block_spent_outputs, size_t transaction_index)
{
    assert(transaction_index < btck_BlockSpentOutputs::get(block_spent_outputs)->vtxundo.size());
    const auto* tx_undo{&btck_BlockSpentOutputs::get(block_spent_outputs)->vtxundo.at(transaction_index)};
    return btck_TransactionSpentOutputs::ref(tx_undo);
}

void btck_block_spent_outputs_destroy(btck_BlockSpentOutputs* block_spent_outputs)
{
    delete block_spent_outputs;
}

btck_TransactionSpentOutputs* btck_transaction_spent_outputs_copy(const btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    return btck_TransactionSpentOutputs::copy(transaction_spent_outputs);
}

size_t btck_transaction_spent_outputs_count(const btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    return btck_TransactionSpentOutputs::get(transaction_spent_outputs).vprevout.size();
}

void btck_transaction_spent_outputs_destroy(btck_TransactionSpentOutputs* transaction_spent_outputs)
{
    delete transaction_spent_outputs;
}

const btck_Coin* btck_transaction_spent_outputs_get_coin_at(const btck_TransactionSpentOutputs* transaction_spent_outputs, size_t coin_index)
{
    assert(coin_index < btck_TransactionSpentOutputs::get(transaction_spent_outputs).vprevout.size());
    const Coin* coin{&btck_TransactionSpentOutputs::get(transaction_spent_outputs).vprevout.at(coin_index)};
    return btck_Coin::ref(coin);
}

btck_Coin* btck_coin_copy(const btck_Coin* coin)
{
    return btck_Coin::copy(coin);
}

uint32_t btck_coin_confirmation_height(const btck_Coin* coin)
{
    return btck_Coin::get(coin).nHeight;
}

int btck_coin_is_coinbase(const btck_Coin* coin)
{
    return btck_Coin::get(coin).IsCoinBase() ? 1 : 0;
}

const btck_TransactionOutput* btck_coin_get_output(const btck_Coin* coin)
{
    return btck_TransactionOutput::ref(&btck_Coin::get(coin).out);
}

void btck_coin_destroy(btck_Coin* coin)
{
    delete coin;
}

int btck_chainstate_manager_process_block(
    btck_ChainstateManager* chainman,
    const btck_Block* block,
    int* _new_block)
{
    bool new_block;
    auto result = btck_ChainstateManager::get(chainman).m_chainman->ProcessNewBlock(btck_Block::get(block), /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
    if (_new_block) {
        *_new_block = new_block ? 1 : 0;
    }
    return result ? 0 : -1;
}

const btck_Chain* btck_chainstate_manager_get_active_chain(const btck_ChainstateManager* chainman)
{
    return btck_Chain::ref(&WITH_LOCK(btck_ChainstateManager::get(chainman).m_chainman->GetMutex(), return btck_ChainstateManager::get(chainman).m_chainman->ActiveChain()));
}

const btck_BlockTreeEntry* btck_chain_get_tip(const btck_Chain* chain)
{
    return btck_BlockTreeEntry::ref(btck_Chain::get(chain).Tip());
}

int btck_chain_get_height(const btck_Chain* chain)
{
    return btck_Chain::get(chain).Height();
}

const btck_BlockTreeEntry* btck_chain_get_genesis(const btck_Chain* chain)
{
    return btck_BlockTreeEntry::ref(btck_Chain::get(chain).Genesis());
}

const btck_BlockTreeEntry* btck_chain_get_by_height(const btck_Chain* chain, int height)
{
    LOCK(::cs_main);
    return btck_BlockTreeEntry::ref(btck_Chain::get(chain)[height]);
}

int btck_chain_contains(const btck_Chain* chain, const btck_BlockTreeEntry* entry)
{
    LOCK(::cs_main);
    return btck_Chain::get(chain).Contains(&btck_BlockTreeEntry::get(entry)) ? 1 : 0;
}
