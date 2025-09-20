// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
#define BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H

#include <kernel/bitcoinkernel.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace btck {

class Transaction;
class TransactionOutput;

enum class LogCategory : btck_LogCategory {
    ALL = btck_LogCategory_ALL,
    BENCH = btck_LogCategory_BENCH,
    BLOCKSTORAGE = btck_LogCategory_BLOCKSTORAGE,
    COINDB = btck_LogCategory_COINDB,
    LEVELDB = btck_LogCategory_LEVELDB,
    MEMPOOL = btck_LogCategory_MEMPOOL,
    PRUNE = btck_LogCategory_PRUNE,
    RAND = btck_LogCategory_RAND,
    REINDEX = btck_LogCategory_REINDEX,
    VALIDATION = btck_LogCategory_VALIDATION,
    KERNEL = btck_LogCategory_KERNEL
};

enum class LogLevel : btck_LogLevel {
    TRACE_LEVEL = btck_LogLevel_TRACE,
    DEBUG_LEVEL = btck_LogLevel_DEBUG,
    INFO_LEVEL = btck_LogLevel_INFO
};

enum class ChainType : btck_ChainType {
    MAINNET = btck_ChainType_MAINNET,
    TESTNET = btck_ChainType_TESTNET,
    TESTNET_4 = btck_ChainType_TESTNET_4,
    SIGNET = btck_ChainType_SIGNET,
    REGTEST = btck_ChainType_REGTEST
};

enum class SynchronizationState : btck_SynchronizationState {
    INIT_REINDEX = btck_SynchronizationState_INIT_REINDEX,
    INIT_DOWNLOAD = btck_SynchronizationState_INIT_DOWNLOAD,
    POST_INIT = btck_SynchronizationState_POST_INIT
};

enum class Warning : btck_Warning {
    UNKNOWN_NEW_RULES_ACTIVATED = btck_Warning_UNKNOWN_NEW_RULES_ACTIVATED,
    LARGE_WORK_INVALID_CHAIN = btck_Warning_LARGE_WORK_INVALID_CHAIN
};

enum class ValidationMode : btck_ValidationMode {
    VALID = btck_ValidationMode_VALID,
    INVALID = btck_ValidationMode_INVALID,
    INTERNAL_ERROR = btck_ValidationMode_INTERNAL_ERROR
};

enum class BlockValidationResult : btck_BlockValidationResult {
    UNSET = btck_BlockValidationResult_UNSET,
    CONSENSUS = btck_BlockValidationResult_CONSENSUS,
    CACHED_INVALID = btck_BlockValidationResult_CACHED_INVALID,
    INVALID_HEADER = btck_BlockValidationResult_INVALID_HEADER,
    MUTATED = btck_BlockValidationResult_MUTATED,
    MISSING_PREV = btck_BlockValidationResult_MISSING_PREV,
    INVALID_PREV = btck_BlockValidationResult_INVALID_PREV,
    TIME_FUTURE = btck_BlockValidationResult_TIME_FUTURE,
    HEADER_LOW_WORK = btck_BlockValidationResult_HEADER_LOW_WORK
};

enum class ScriptVerifyStatus : btck_ScriptVerifyStatus {
    OK = btck_ScriptVerifyStatus_SCRIPT_VERIFY_OK,
    ERROR_INVALID_FLAGS_COMBINATION = btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION,
    ERROR_SPENT_OUTPUTS_REQUIRED = btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED,
};

enum class ScriptVerificationFlags : btck_ScriptVerificationFlags {
    NONE = btck_ScriptVerificationFlags_NONE,
    P2SH = btck_ScriptVerificationFlags_P2SH,
    DERSIG = btck_ScriptVerificationFlags_DERSIG,
    NULLDUMMY = btck_ScriptVerificationFlags_NULLDUMMY,
    CHECKLOCKTIMEVERIFY = btck_ScriptVerificationFlags_CHECKLOCKTIMEVERIFY,
    CHECKSEQUENCEVERIFY = btck_ScriptVerificationFlags_CHECKSEQUENCEVERIFY,
    WITNESS = btck_ScriptVerificationFlags_WITNESS,
    TAPROOT = btck_ScriptVerificationFlags_TAPROOT,
    ALL = btck_ScriptVerificationFlags_ALL
};

template <typename T>
struct is_bitmask_enum : std::false_type {
};

template <>
struct is_bitmask_enum<ScriptVerificationFlags> : std::true_type {
};

template <typename T>
concept BitmaskEnum = is_bitmask_enum<T>::value;

template <BitmaskEnum T>
constexpr T operator|(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) | static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator&(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) & static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator^(T lhs, T rhs)
{
    return static_cast<T>(
        static_cast<std::underlying_type_t<T>>(lhs) ^ static_cast<std::underlying_type_t<T>>(rhs));
}

template <BitmaskEnum T>
constexpr T operator~(T value)
{
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(value));
}

template <BitmaskEnum T>
constexpr T& operator|=(T& lhs, T rhs)
{
    return lhs = lhs | rhs;
}

template <BitmaskEnum T>
constexpr T& operator&=(T& lhs, T rhs)
{
    return lhs = lhs & rhs;
}

template <BitmaskEnum T>
constexpr T& operator^=(T& lhs, T rhs)
{
    return lhs = lhs ^ rhs;
}

template <typename T>
T check(T ptr)
{
    if (ptr == nullptr) {
        throw std::runtime_error("failed to instantiate btck object");
    }
    return ptr;
}

template <typename Collection, typename ValueType>
class Iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;

private:
    const Collection* m_collection;
    size_t m_idx;

public:
    Iterator() = default;
    Iterator(const Collection* ptr) : m_collection{ptr}, m_idx{0} {}
    Iterator(const Collection* ptr, size_t idx) : m_collection{ptr}, m_idx{idx} {}

    auto operator*() const { return (*m_collection)[m_idx]; }
    auto operator->() const { return (*m_collection)[m_idx]; }

    auto& operator++() { m_idx++; return *this; }
    auto operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

    auto& operator--() { m_idx--; return *this; }
    auto operator--(int) { auto temp = *this; --m_idx; return temp; }

    auto& operator+=(difference_type n) { m_idx += n; return *this; }
    auto& operator-=(difference_type n) { m_idx -= n; return *this; }

    auto operator+(difference_type n) const { return Iterator(m_collection, m_idx + n); }
    auto operator-(difference_type n) const { return Iterator(m_collection, m_idx - n); }

    auto operator-(const Iterator& other) const { return static_cast<difference_type>(m_idx) - static_cast<difference_type>(other.m_idx); }

    ValueType operator[](difference_type n) const { return (*m_collection)[m_idx + n]; }

    auto operator<=>(const Iterator& other) const { return m_idx <=> other.m_idx; }

    bool operator==(const Iterator& other) const { return m_collection == other.m_collection && m_idx == other.m_idx; }

private:
    friend Iterator operator+(difference_type n, const Iterator& it) { return it + n; }
};

template <typename Container, typename SizeFunc, typename GetFunc>
concept IndexedContainer = requires(const Container& c, SizeFunc size_func, GetFunc get_func, std::size_t i) {
    { std::invoke(size_func, c) } -> std::convertible_to<std::size_t>;
    { std::invoke(get_func, c, i) }; // Return type is deduced
};

template <typename Container, auto SizeFunc, auto GetFunc>
    requires IndexedContainer<Container, decltype(SizeFunc), decltype(GetFunc)>
class Range
{
public:
    using value_type = std::invoke_result_t<decltype(GetFunc), const Container&, size_t>;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator<Range, value_type>;
    using const_iterator = iterator;

private:
    const Container* m_container;

public:
    explicit Range(const Container& container) : m_container(&container)
    {
        static_assert(std::ranges::random_access_range<Range>);
    }

    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, size()); }

    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }

    size_t size() const { return std::invoke(SizeFunc, *m_container); }

    bool empty() const { return size() == 0; }

    value_type operator[](size_t index) const { return std::invoke(GetFunc, *m_container, index); }

    value_type at(size_t index) const
    {
        if (index >= size()) {
            throw std::out_of_range("Index out of range");
        }
        return (*this)[index];
    }

    value_type front() const { return (*this)[0]; }
    value_type back() const { return (*this)[size() - 1]; }
};

template <typename T>
std::vector<std::byte> write_bytes(const T* object, int (*to_bytes)(const T*, btck_WriteBytes, void*))
{
    std::vector<std::byte> bytes;
    struct UserData {
        std::vector<std::byte>* bytes;
        std::exception_ptr exception;
    };
    UserData user_data = UserData{.bytes = &bytes, .exception = nullptr};

    constexpr auto const write = +[](const void* buffer, size_t len, void* user_data) -> int {
        auto& data = *reinterpret_cast<UserData*>(user_data);
        auto& bytes = *data.bytes;
        try {
            auto const* first = static_cast<const std::byte*>(buffer);
            auto const* last = first + len;
            bytes.insert(bytes.end(), first, last);
            return 0;
        } catch (...) {
            data.exception = std::current_exception();
            return -1;
        }
    };

    if (to_bytes(object, write, &user_data) != 0) {
        std::rethrow_exception(user_data.exception);
    }
    return bytes;
}

template <typename CType>
class View
{
protected:
    const CType* m_ptr;

public:
    explicit View(const CType* ptr) : m_ptr{check(ptr)} {}

    const CType* get() const { return m_ptr; }
};

template <typename CType, CType* (*CopyFunc)(const CType*), void (*DestroyFunc)(CType*)>
class Handle
{
protected:
    CType* m_ptr;

public:
    explicit Handle(CType* ptr) : m_ptr{check(ptr)} {}

    // Copy constructors
    Handle(const Handle& other)
        : m_ptr{check(CopyFunc(other.m_ptr))} {}
    Handle operator=(const Handle& other)
    {
        if (this != &other) {
            Handle temp(other);
            std::swap(m_ptr, temp.m_ptr);
        }
        return *this;
    }

    // Move constructors
    Handle(Handle&& other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
    Handle operator=(Handle&& other) noexcept
    {
        DestroyFunc(m_ptr);
        m_ptr = std::exchange(other.m_ptr, nullptr);
        return *this;
    }

    template <typename ViewType>
        requires std::derived_from<ViewType, View<CType>>
    Handle(const ViewType& view)
        : Handle{CopyFunc(view.get())}
    {
    }

    ~Handle() { DestroyFunc(m_ptr); }

    CType* get() { return m_ptr; }
    const CType* get() const { return m_ptr; }
};

template <typename CType, void (*DestroyFunc)(CType*)>
class UniqueHandle
{
protected:
    struct Deleter {
        void operator()(CType* ptr) const noexcept
        {
            if (ptr) DestroyFunc(ptr);
        }
    };
    std::unique_ptr<CType, Deleter> m_ptr;

public:
    explicit UniqueHandle(CType* ptr) : m_ptr{check(ptr)} {}

    CType* get() { return m_ptr.get(); }
    const CType* get() const { return m_ptr.get(); }
};

class Transaction;
class TransactionOutput;

template <typename Derived>
class ScriptPubkeyApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    ScriptPubkeyApi() = default;

public:
    bool Verify(int64_t amount,
                const Transaction& tx_to,
                std::span<const TransactionOutput> spent_outputs,
                unsigned int input_index,
                ScriptVerificationFlags flags,
                ScriptVerifyStatus& status) const;

    std::vector<std::byte> ToBytes() const
    {
        return write_bytes(impl(), btck_script_pubkey_to_bytes);
    }
};

class ScriptPubkeyView : public View<btck_ScriptPubkey>, public ScriptPubkeyApi<ScriptPubkeyView>
{
public:
    explicit ScriptPubkeyView(const btck_ScriptPubkey* ptr) : View{ptr} {}
};

class ScriptPubkey : public Handle<btck_ScriptPubkey, btck_script_pubkey_copy, btck_script_pubkey_destroy>, public ScriptPubkeyApi<ScriptPubkey>
{
public:
    explicit ScriptPubkey(std::span<const std::byte> raw)
        : Handle{btck_script_pubkey_create(raw.data(), raw.size())} {}

    ScriptPubkey(const ScriptPubkeyView& view)
        : Handle(view) {}
};

template <typename Derived>
class TransactionOutputApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    TransactionOutputApi() = default;

public:
    int64_t Amount() const
    {
        return btck_transaction_output_get_amount(impl());
    }

    ScriptPubkeyView GetScriptPubkey() const
    {
        return ScriptPubkeyView{btck_transaction_output_get_script_pubkey(impl())};
    }
};

class TransactionOutputView : public View<btck_TransactionOutput>, public TransactionOutputApi<TransactionOutputView>
{
public:
    explicit TransactionOutputView(const btck_TransactionOutput* ptr) : View{ptr} {}
};

class TransactionOutput : public Handle<btck_TransactionOutput, btck_transaction_output_copy, btck_transaction_output_destroy>, public TransactionOutputApi<TransactionOutput>
{
public:
    explicit TransactionOutput(const ScriptPubkey& script_pubkey, int64_t amount)
        : Handle{btck_transaction_output_create(script_pubkey.get(), amount)} {}

    TransactionOutput(const TransactionOutputView& view)
        : Handle(view) {}
};

template <typename Derived>
class TransactionApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

public:
    size_t CountOutputs() const
    {
        return btck_transaction_count_outputs(impl());
    }

    size_t CountInputs() const
    {
        return btck_transaction_count_inputs(impl());
    }

    TransactionOutputView GetOutput(size_t index) const
    {
        return TransactionOutputView{btck_transaction_get_output_at(impl(), index)};
    }

    auto Outputs() const
    {
        return Range<Derived, &TransactionApi<Derived>::CountOutputs, &TransactionApi<Derived>::GetOutput>{*static_cast<const Derived*>(this)};
    }

    std::vector<std::byte> ToBytes() const
    {
        return write_bytes(impl(), btck_transaction_to_bytes);
    }
};

class TransactionView : public View<btck_Transaction>, public TransactionApi<TransactionView>
{
public:
    explicit TransactionView(const btck_Transaction* ptr) : View{ptr} {}
};

class Transaction : public Handle<btck_Transaction, btck_transaction_copy, btck_transaction_destroy>, public TransactionApi<Transaction>
{
public:
    explicit Transaction(std::span<const std::byte> raw_transaction)
        : Handle{btck_transaction_create(raw_transaction.data(), raw_transaction.size())} {}

    Transaction(const TransactionView& view)
        : Handle{view} {}
};

template <typename Derived>
bool ScriptPubkeyApi<Derived>::Verify(int64_t amount,
                                      const Transaction& tx_to,
                                      const std::span<const TransactionOutput> spent_outputs,
                                      unsigned int input_index,
                                      ScriptVerificationFlags flags,
                                      ScriptVerifyStatus& status) const
{
    const btck_TransactionOutput** spent_outputs_ptr = nullptr;
    std::vector<const btck_TransactionOutput*> raw_spent_outputs;
    if (spent_outputs.size() > 0) {
        raw_spent_outputs.reserve(spent_outputs.size());

        for (const auto& output : spent_outputs) {
            raw_spent_outputs.push_back(output.get());
        }
        spent_outputs_ptr = raw_spent_outputs.data();
    }
    auto result = btck_script_pubkey_verify(
        impl(),
        amount,
        tx_to.get(),
        spent_outputs_ptr, spent_outputs.size(),
        input_index,
        static_cast<btck_ScriptVerificationFlags>(flags),
        reinterpret_cast<btck_ScriptVerifyStatus*>(&status));
    return result == 1;
}

class BlockHash : public Handle<btck_BlockHash, btck_block_hash_copy, btck_block_hash_destroy>
{
public:
    explicit BlockHash(const std::array<std::byte, 32>& hash)
        : Handle{btck_block_hash_create(reinterpret_cast<const unsigned char*>(hash.data()))} {}

    explicit BlockHash(btck_BlockHash* hash)
        : Handle{hash} {}

    std::array<std::byte, 32> Bytes() const
    {
        std::array<std::byte, 32> hash;
        btck_block_hash_to_bytes(get(), reinterpret_cast<unsigned char*>(hash.data()));
        return hash;
    }
};

class Block : public Handle<btck_Block, btck_block_copy, btck_block_destroy>
{
public:
    Block(const std::span<const std::byte> raw_block)
        : Handle{check(btck_block_create(raw_block.data(), raw_block.size()))}
    {
    }

    Block(btck_Block* block) : Handle{check(block)} {}

    size_t CountTransactions() const
    {
        return btck_block_count_transactions(get());
    }

    TransactionView GetTransaction(size_t index) const
    {
        return TransactionView{btck_block_get_transaction_at(get(), index)};
    }

    auto Transactions() const
    {
        return Range<Block, &Block::CountTransactions, &Block::GetTransaction>{*this};
    }

    BlockHash GetHash() const
    {
        return BlockHash{btck_block_get_hash(get())};
    }

    std::vector<std::byte> ToBytes() const
    {
        return write_bytes(get(), btck_block_to_bytes);
    }

    friend class ChainMan;
};

void logging_disable()
{
    btck_logging_disable();
}

void logging_set_level_category(LogCategory category, LogLevel level)
{
    btck_logging_set_level_category(static_cast<btck_LogCategory>(category), static_cast<btck_LogLevel>(level));
}

void logging_enable_category(LogCategory category)
{
    btck_logging_enable_category(static_cast<btck_LogCategory>(category));
}

void logging_disable_category(LogCategory category)
{
    btck_logging_disable_category(static_cast<btck_LogCategory>(category));
}

template <typename T>
concept Log = requires(T a, std::string_view message) {
    { a.LogMessage(message) } -> std::same_as<void>;
};

template <Log T>
class Logger : UniqueHandle<btck_LoggingConnection, btck_logging_connection_destroy>
{
public:
    Logger(std::unique_ptr<T> log, const btck_LoggingOptions& logging_options)
        : UniqueHandle{btck_logging_connection_create(
              +[](void* user_data, const char* message, size_t message_len) { static_cast<T*>(user_data)->LogMessage({message, message_len}); },
              log.release(),
              +[](void* user_data) { delete static_cast<T*>(user_data); },
              logging_options)}
    {
    }
};

class BlockTreeEntry : public View<btck_BlockTreeEntry>
{
public:
    BlockTreeEntry(const btck_BlockTreeEntry* entry)
        : View{check(entry)}
    {
    }

    std::optional<BlockTreeEntry> GetPrevious() const
    {
        auto entry{btck_block_tree_entry_get_previous(get())};
        if (!entry) return std::nullopt;
        return entry;
    }

    int32_t GetHeight() const
    {
        return btck_block_tree_entry_get_height(get());
    }

    BlockHash GetHash() const
    {
        return BlockHash{btck_block_tree_entry_get_block_hash(get())};
    }

    friend class ChainMan;
    friend class Chain;
};

template <typename T>
class KernelNotifications
{
public:
    virtual ~KernelNotifications() = default;

    virtual void BlockTipHandler(SynchronizationState state, BlockTreeEntry entry, double verification_progress) {}

    virtual void HeaderTipHandler(SynchronizationState state, int64_t height, int64_t timestamp, bool presync) {}

    virtual void ProgressHandler(std::string_view title, int progress_percent, bool resume_possible) {}

    virtual void WarningSetHandler(Warning warning, std::string_view message) {}

    virtual void WarningUnsetHandler(Warning warning) {}

    virtual void FlushErrorHandler(std::string_view error) {}

    virtual void FatalErrorHandler(std::string_view error) {}
};

class BlockValidationState
{
private:
    const btck_BlockValidationState* m_state;

public:
    BlockValidationState(const btck_BlockValidationState* state) : m_state{state} {}

    BlockValidationState(const BlockValidationState&) = delete;
    BlockValidationState& operator=(const BlockValidationState&) = delete;
    BlockValidationState(BlockValidationState&&) = delete;
    BlockValidationState& operator=(BlockValidationState&&) = delete;

    ValidationMode GetValidationMode() const
    {
        return static_cast<ValidationMode>(btck_block_validation_state_get_validation_mode(m_state));
    }

    BlockValidationResult GetBlockValidationResult() const
    {
        return static_cast<BlockValidationResult>(btck_block_validation_state_get_block_validation_result(m_state));
    }
};

template <typename T>
class ValidationInterface
{
public:
    virtual ~ValidationInterface() = default;

    virtual void BlockChecked(Block block, const BlockValidationState state) {}
};

class ChainParams : Handle<btck_ChainParameters, btck_chain_parameters_copy, btck_chain_parameters_destroy>
{
public:
    ChainParams(ChainType chain_type)
        : Handle{btck_chain_parameters_create(static_cast<btck_ChainType>(chain_type))} {}

    friend class ContextOptions;
};

class ContextOptions : UniqueHandle<btck_ContextOptions, btck_context_options_destroy>
{
public:
    ContextOptions() : UniqueHandle{check(btck_context_options_create())} {}

    void SetChainParams(ChainParams& chain_params)
    {
        btck_context_options_set_chainparams(get(), chain_params.get());
    }

    template <typename T>
    void SetNotifications(std::shared_ptr<T> notifications)
    {
        static_assert(std::is_base_of_v<KernelNotifications<T>, T>);
        auto heap_notifications = std::make_unique<std::shared_ptr<T>>(std::move(notifications));
        using user_type = std::shared_ptr<T>*;
        btck_context_options_set_notifications(
            get(),
            btck_NotificationInterfaceCallbacks{
                .user_data = heap_notifications.release(),
                .user_data_destroy = +[](void* user_data) { delete static_cast<user_type>(user_data); },
                .block_tip = +[](void* user_data, btck_SynchronizationState state, const btck_BlockTreeEntry* entry, double verification_progress) { (*static_cast<user_type>(user_data))->BlockTipHandler(static_cast<SynchronizationState>(state), BlockTreeEntry{entry}, verification_progress); },
                .header_tip = +[](void* user_data, btck_SynchronizationState state, int64_t height, int64_t timestamp, int presync) { (*static_cast<user_type>(user_data))->HeaderTipHandler(static_cast<SynchronizationState>(state), height, timestamp, presync == 1); },
                .progress = +[](void* user_data, const char* title, size_t title_len, int progress_percent, int resume_possible) { (*static_cast<user_type>(user_data))->ProgressHandler({title, title_len}, progress_percent, resume_possible == 1); },
                .warning_set = +[](void* user_data, btck_Warning warning, const char* message, size_t message_len) { (*static_cast<user_type>(user_data))->WarningSetHandler(static_cast<Warning>(warning), {message, message_len}); },
                .warning_unset = +[](void* user_data, btck_Warning warning) { (*static_cast<user_type>(user_data))->WarningUnsetHandler(static_cast<Warning>(warning)); },
                .flush_error = +[](void* user_data, const char* error, size_t error_len) { (*static_cast<user_type>(user_data))->FlushErrorHandler({error, error_len}); },
                .fatal_error = +[](void* user_data, const char* error, size_t error_len) { (*static_cast<user_type>(user_data))->FatalErrorHandler({error, error_len}); },
            });
    }

    template <typename T>
    void SetValidationInterface(std::shared_ptr<T> validation_interface)
    {
        static_assert(std::is_base_of_v<ValidationInterface<T>, T>);
        auto heap_vi = std::make_unique<std::shared_ptr<T>>(std::move(validation_interface));
        using user_type = std::shared_ptr<T>*;
        btck_context_options_set_validation_interface(
            get(),
            btck_ValidationInterfaceCallbacks{
                .user_data = heap_vi.release(),
                .user_data_destroy = +[](void* user_data) { delete static_cast<user_type>(user_data); },
                .block_checked = +[](void* user_data, btck_Block* block, const btck_BlockValidationState* state) { (*static_cast<user_type>(user_data))->BlockChecked(Block{block}, BlockValidationState{state}); },
            });
    }

    friend class Context;
};

class Context : public Handle<btck_Context, btck_context_copy, btck_context_destroy>
{
public:
    Context(ContextOptions& opts)
        : Handle{btck_context_create(opts.get())} {}

    Context()
        : Handle{check(btck_context_create(ContextOptions{}.get()))} {}

    bool interrupt() {
        return btck_context_interrupt(get()) == 0;
    }

    friend class ChainstateManagerOptions;
};

class ChainstateManagerOptions : UniqueHandle<btck_ChainstateManagerOptions, btck_chainstate_manager_options_destroy>
{
public:
    ChainstateManagerOptions(const Context& context, const std::string& data_dir, const std::string& blocks_dir)
        : UniqueHandle{check(btck_chainstate_manager_options_create(context.get(), data_dir.c_str(), data_dir.length(), blocks_dir.c_str(), blocks_dir.length()))}
    {
    }

    void SetWorkerThreads(int worker_threads)
    {
        btck_chainstate_manager_options_set_worker_threads_num(get(), worker_threads);
    }

    bool SetWipeDbs(bool wipe_block_tree, bool wipe_chainstate)
    {
        return btck_chainstate_manager_options_set_wipe_dbs(get(), wipe_block_tree, wipe_chainstate) == 0;
    }

    void SetBlockTreeDbInMemory(bool block_tree_db_in_memory)
    {
        btck_chainstate_manager_options_set_block_tree_db_in_memory(get(), block_tree_db_in_memory);
    }

    void SetChainstateDbInMemory(bool chainstate_db_in_memory)
    {
        btck_chainstate_manager_options_set_chainstate_db_in_memory(get(), chainstate_db_in_memory);
    }

    friend class ChainMan;
};

class ChainView : public View<btck_Chain>
{
public:
    explicit ChainView(const btck_Chain* ptr) : View{ptr} {}

    BlockTreeEntry Tip() const
    {
        return btck_chain_get_tip(get());
    }

    int Height() const
    {
        return btck_chain_get_height(get());
    }

    BlockTreeEntry Genesis() const
    {
        return btck_chain_get_genesis(get());
    }

    BlockTreeEntry GetByHeight(int height) const
    {
        auto index{btck_chain_get_by_height(get(), height)};
        if (!index) throw std::runtime_error("No entry in the chain at the provided height");
        return index;
    }

    bool Contains(BlockTreeEntry& entry) const
    {
        return btck_chain_contains(get(), entry.get());
    }

    auto Entries() const
    {
        return Range<ChainView, &ChainView::Height, &ChainView::GetByHeight>{*this};
    }
};

template <typename Derived>
class CoinApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    CoinApi() = default;

public:
    uint32_t GetConfirmationHeight() const { return btck_coin_confirmation_height(impl()); }

    bool IsCoinbase() const { return btck_coin_is_coinbase(impl()) == 1; }

    TransactionOutputView GetOutput() const
    {
        return TransactionOutputView{btck_coin_get_output(impl())};
    }
};

class CoinView : public View<btck_Coin>, public CoinApi<CoinView>
{
public:
    explicit CoinView(const btck_Coin* ptr) : View{ptr} {}
};

class Coin : Handle<btck_Coin, btck_coin_copy, btck_coin_destroy>, public CoinApi<Coin>
{
public:
    Coin(btck_Coin* coin) : Handle{check(coin)} {}

    Coin(const CoinView& view) : Handle{view} {}
};

template <typename Derived>
class TransactionSpentOutputsApi
{
private:
    auto impl() const
    {
        return static_cast<const Derived*>(this)->get();
    }

    friend Derived;
    TransactionSpentOutputsApi() = default;

public:
    size_t Count() const
    {
        return btck_transaction_spent_outputs_count(impl());
    }

    CoinView GetCoin(size_t index) const
    {
        return CoinView{btck_transaction_spent_outputs_get_coin_at(impl(), index)};
    }

    auto Coins() const
    {
        return Range<Derived, &TransactionSpentOutputsApi<Derived>::Count, &TransactionSpentOutputsApi<Derived>::GetCoin>{*static_cast<const Derived*>(this)};
    }
};

class TransactionSpentOutputsView : public View<btck_TransactionSpentOutputs>, public TransactionSpentOutputsApi<TransactionSpentOutputsView>
{
public:
    explicit TransactionSpentOutputsView(const btck_TransactionSpentOutputs* ptr) : View{ptr} {}
};

class TransactionSpentOutputs : Handle<btck_TransactionSpentOutputs, btck_transaction_spent_outputs_copy, btck_transaction_spent_outputs_destroy>,
                                public TransactionSpentOutputsApi<TransactionSpentOutputs>
{
public:
    TransactionSpentOutputs(btck_TransactionSpentOutputs* transaction_spent_outputs) : Handle{check(transaction_spent_outputs)} {}

    TransactionSpentOutputs(const TransactionSpentOutputsView& view) : Handle{view} {}
};

class BlockSpentOutputs : Handle<btck_BlockSpentOutputs, btck_block_spent_outputs_copy, btck_block_spent_outputs_destroy>
{
public:
    BlockSpentOutputs(btck_BlockSpentOutputs* block_spent_outputs)
        : Handle{check(block_spent_outputs)}
    {
    }

    size_t Count() const
    {
        return btck_block_spent_outputs_count(get());
    }

    TransactionSpentOutputsView GetTxSpentOutputs(size_t tx_undo_index) const
    {
        return TransactionSpentOutputsView{btck_block_spent_outputs_get_transaction_spent_outputs_at(get(), tx_undo_index)};
    }

    auto TxsSpentOutputs() const
    {
        return Range<BlockSpentOutputs, &BlockSpentOutputs::Count, &BlockSpentOutputs::GetTxSpentOutputs>{*this};
    }
};

class ChainMan : UniqueHandle<btck_ChainstateManager, btck_chainstate_manager_destroy>
{
public:
    ChainMan(const Context& context, const ChainstateManagerOptions& chainman_opts)
        : UniqueHandle{check(btck_chainstate_manager_create(chainman_opts.get()))}
    {
    }

    bool ImportBlocks(const std::span<const std::string> paths)
    {
        std::vector<const char*> c_paths;
        std::vector<size_t> c_paths_lens;
        c_paths.reserve(paths.size());
        c_paths_lens.reserve(paths.size());
        for (const auto& path : paths) {
            c_paths.push_back(path.c_str());
            c_paths_lens.push_back(path.length());
        }

        return btck_chainstate_manager_import_blocks(get(), c_paths.data(), c_paths_lens.data(), c_paths.size()) == 0;
    }

    bool ProcessBlock(const Block& block, bool* new_block)
    {
        int _new_block;
        int res = btck_chainstate_manager_process_block(get(), block.get(), &_new_block);
        if (new_block) *new_block = _new_block == 1;
        return res == 0;
    }

    ChainView GetChain() const
    {
        return ChainView{btck_chainstate_manager_get_active_chain(get())};
    }

    BlockTreeEntry GetBlockTreeEntry(const btck_BlockHash* block_hash) const
    {
        return btck_chainstate_manager_get_block_tree_entry_by_hash(get(), block_hash);
    }

    std::optional<Block> ReadBlock(const BlockTreeEntry& entry) const
    {
        auto block{btck_block_read(get(), entry.get())};
        if (!block) return std::nullopt;
        return block;
    }

    BlockSpentOutputs ReadBlockSpentOutputs(const BlockTreeEntry& entry) const
    {
        return btck_block_spent_outputs_read(get(), entry.get());
    }
};

} // namespace btck

#endif // BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
