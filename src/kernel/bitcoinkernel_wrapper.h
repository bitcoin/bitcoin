// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
#define BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H

#include <kernel/bitcoinkernel.h>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class Transaction
{
private:
    struct Deleter {
        void operator()(kernel_Transaction* ptr) const
        {
            kernel_transaction_destroy(ptr);
        }
    };

public:
    std::unique_ptr<kernel_Transaction, Deleter> m_transaction;

    Transaction(std::span<const unsigned char> raw_transaction) noexcept
        : m_transaction{kernel_transaction_create(raw_transaction.data(), raw_transaction.size())}
    {
    }

    /** Check whether this Transaction object is valid. */
    explicit operator bool() const noexcept { return bool{m_transaction}; }
};

class ScriptPubkey
{
private:
    struct Deleter {
        void operator()(kernel_ScriptPubkey* ptr) const
        {
            kernel_script_pubkey_destroy(ptr);
        }
    };

public:
    std::unique_ptr<kernel_ScriptPubkey, Deleter> m_script_pubkey;

    ScriptPubkey(std::span<const unsigned char> script_pubkey) noexcept
        : m_script_pubkey{kernel_script_pubkey_create(script_pubkey.data(), script_pubkey.size())}
    {
    }

    ScriptPubkey(kernel_ScriptPubkey* script_pubkey) noexcept
        : m_script_pubkey{script_pubkey}
    {
    }

    std::vector<unsigned char> GetScriptPubkeyData() const noexcept
    {
        auto serialized_data{kernel_copy_script_pubkey_data(m_script_pubkey.get())};
        std::vector<unsigned char> vec{serialized_data->data, serialized_data->data + serialized_data->size};
        kernel_byte_array_destroy(serialized_data);
        return vec;
    }

    /** Check whether this ScriptPubkey object is valid. */
    explicit operator bool() const noexcept { return bool{m_script_pubkey}; }
};

class TransactionOutput
{
private:
    struct Deleter {
        void operator()(kernel_TransactionOutput* ptr) const
        {
            kernel_transaction_output_destroy(ptr);
        }
    };

public:
    std::unique_ptr<kernel_TransactionOutput, Deleter> m_transaction_output;

    TransactionOutput(const ScriptPubkey& script_pubkey, int64_t amount) noexcept
        : m_transaction_output{kernel_transaction_output_create(script_pubkey.m_script_pubkey.get(), amount)}
    {
    }

    TransactionOutput(kernel_TransactionOutput* output) noexcept
        : m_transaction_output{output}
    {
    }

    /** Check whether this TransactionOutput object is valid. */
    explicit operator bool() const noexcept { return bool{m_transaction_output}; }

    ScriptPubkey GetScriptPubkey() noexcept
    {
        return kernel_copy_script_pubkey_from_output(m_transaction_output.get());
    }

    int64_t GetOutputAmount() noexcept
    {
        return kernel_get_transaction_output_amount(m_transaction_output.get());
    }
};

int verify_script(const ScriptPubkey& script_pubkey,
                  int64_t amount,
                  const Transaction& tx_to,
                  const std::span<const TransactionOutput> spent_outputs,
                  unsigned int input_index,
                  unsigned int flags,
                  kernel_ScriptVerifyStatus& status) noexcept
{
    const kernel_TransactionOutput** spent_outputs_ptr = nullptr;
    std::vector<const kernel_TransactionOutput*> raw_spent_outputs;
    if (spent_outputs.size() > 0) {
        raw_spent_outputs.reserve(spent_outputs.size());

        for (const auto& output : spent_outputs) {
            raw_spent_outputs.push_back(output.m_transaction_output.get());
        }
        spent_outputs_ptr = raw_spent_outputs.data();
    }
    return kernel_verify_script(
        script_pubkey.m_script_pubkey.get(),
        amount,
        tx_to.m_transaction.get(),
        spent_outputs_ptr, spent_outputs.size(),
        input_index,
        flags,
        &status);
}

template <typename T>
concept Log = requires(T a, std::string_view message) {
    { a.LogMessage(message) } -> std::same_as<void>;
};

template <Log T>
class Logger
{
private:
    struct Deleter {
        void operator()(kernel_LoggingConnection* ptr) const
        {
            kernel_logging_connection_destroy(ptr);
        }
    };

    std::unique_ptr<T> m_log;
    std::unique_ptr<kernel_LoggingConnection, Deleter> m_connection;

public:
    Logger(std::unique_ptr<T> log, const kernel_LoggingOptions& logging_options) noexcept
        : m_log{std::move(log)},
          m_connection{kernel_logging_connection_create(
              [](void* user_data, const char* message, size_t message_len) { static_cast<T*>(user_data)->LogMessage({message, message_len}); },
              m_log.get(),
              logging_options)}
    {
    }

    /** Check whether this Logger object is valid. */
    explicit operator bool() const noexcept { return bool{m_connection}; }
};

template <typename T>
class KernelNotifications
{
private:
    kernel_NotificationInterfaceCallbacks MakeCallbacks()
    {
        return kernel_NotificationInterfaceCallbacks{
            .user_data = this,
            .block_tip = [](void* user_data, kernel_SynchronizationState state, const kernel_BlockIndex* index) {
                static_cast<T*>(user_data)->BlockTipHandler(state, index);
            },
            .header_tip = [](void* user_data, kernel_SynchronizationState state, int64_t height, int64_t timestamp, bool presync) {
                static_cast<T*>(user_data)->HeaderTipHandler(state, height, timestamp, presync);
            },
            .progress = [](void* user_data, const char* title, size_t title_len, int progress_percent, bool resume_possible) {
                static_cast<T*>(user_data)->ProgressHandler({title, title_len}, progress_percent, resume_possible);
            },
            .warning_set = [](void* user_data, kernel_Warning warning, const char* message, size_t message_len) {
                static_cast<T*>(user_data)->WarningSetHandler(warning, {message, message_len});
            },
            .warning_unset = [](void* user_data, kernel_Warning warning) { static_cast<T*>(user_data)->WarningUnsetHandler(warning); },
            .flush_error = [](void* user_data, const char* error, size_t error_len) { static_cast<T*>(user_data)->FlushErrorHandler({error, error_len}); },
            .fatal_error = [](void* user_data, const char* error, size_t error_len) { static_cast<T*>(user_data)->FatalErrorHandler({error, error_len}); },
        };
    }

    const kernel_NotificationInterfaceCallbacks m_notifications;

public:
    KernelNotifications() : m_notifications{MakeCallbacks()} {}

    virtual ~KernelNotifications() = default;

    virtual void BlockTipHandler(kernel_SynchronizationState state, const kernel_BlockIndex* index) {}

    virtual void HeaderTipHandler(kernel_SynchronizationState state, int64_t height, int64_t timestamp, bool presync) {}

    virtual void ProgressHandler(std::string_view title, int progress_percent, bool resume_possible) {}

    virtual void WarningSetHandler(kernel_Warning warning, std::string_view message) {}

    virtual void WarningUnsetHandler(kernel_Warning warning) {}

    virtual void FlushErrorHandler(std::string_view error) {}

    virtual void FatalErrorHandler(std::string_view error) {}

    friend class ContextOptions;
};

struct BlockHashDeleter {
    void operator()(kernel_BlockHash* ptr) const
    {
        kernel_block_hash_destroy(ptr);
    }
};

class UnownedBlock
{
private:
    const kernel_BlockPointer* m_block;

public:
    UnownedBlock(const kernel_BlockPointer* block) noexcept : m_block{block} {}

    UnownedBlock(const UnownedBlock&) = delete;
    UnownedBlock& operator=(const UnownedBlock&) = delete;
    UnownedBlock(UnownedBlock&&) = delete;
    UnownedBlock& operator=(UnownedBlock&&) = delete;

    std::unique_ptr<kernel_BlockHash, BlockHashDeleter> GetHash() const noexcept
    {
        return std::unique_ptr<kernel_BlockHash, BlockHashDeleter>(kernel_block_pointer_get_hash(m_block));
    }

    std::vector<unsigned char> GetBlockData() const noexcept
    {
        auto serialized_block{kernel_copy_block_pointer_data(m_block)};
        std::vector<unsigned char> vec{serialized_block->data, serialized_block->data + serialized_block->size};
        kernel_byte_array_destroy(serialized_block);
        return vec;
    }
};

class BlockValidationState
{
private:
    const kernel_BlockValidationState* m_state;

public:
    BlockValidationState(const kernel_BlockValidationState* state) noexcept : m_state{state} {}

    BlockValidationState(const BlockValidationState&) = delete;
    BlockValidationState& operator=(const BlockValidationState&) = delete;
    BlockValidationState(BlockValidationState&&) = delete;
    BlockValidationState& operator=(BlockValidationState&&) = delete;

    kernel_ValidationMode ValidationMode() const noexcept
    {
        return kernel_get_validation_mode_from_block_validation_state(m_state);
    }

    kernel_BlockValidationResult BlockValidationResult() const noexcept
    {
        return kernel_get_block_validation_result_from_block_validation_state(m_state);
    }
};

template <typename T>
class ValidationInterface
{
private:
    const kernel_ValidationInterfaceCallbacks m_validation_interface;

public:
    ValidationInterface() noexcept : m_validation_interface{kernel_ValidationInterfaceCallbacks{
                                .user_data = this,
                                .block_checked = [](void* user_data, const kernel_BlockPointer* block, const kernel_BlockValidationState* state) {
                                    static_cast<T*>(user_data)->BlockChecked(UnownedBlock{block}, BlockValidationState{state});
                                },
                            }}
    {
    }

    virtual ~ValidationInterface() = default;

    virtual void BlockChecked(UnownedBlock block, const BlockValidationState state) {}

    friend class ContextOptions;
};

class ChainParams
{
private:
    struct Deleter {
        void operator()(kernel_ChainParameters* ptr) const
        {
            kernel_chain_parameters_destroy(ptr);
        }
    };

    std::unique_ptr<kernel_ChainParameters, Deleter> m_chain_params;

public:
    ChainParams(kernel_ChainType chain_type) noexcept : m_chain_params{kernel_chain_parameters_create(chain_type)} {}

    friend class ContextOptions;
};

class ContextOptions
{
private:
    struct Deleter {
        void operator()(kernel_ContextOptions* ptr) const
        {
            kernel_context_options_destroy(ptr);
        }
    };

    std::unique_ptr<kernel_ContextOptions, Deleter> m_options;

public:
    ContextOptions() noexcept : m_options{kernel_context_options_create()} {}

    void SetChainParams(ChainParams& chain_params) const noexcept
    {
        kernel_context_options_set_chainparams(m_options.get(), chain_params.m_chain_params.get());
    }

    template <typename T>
    void SetNotifications(KernelNotifications<T>& notifications) const noexcept
    {
        kernel_context_options_set_notifications(m_options.get(), notifications.m_notifications);
    }

    template <typename T>
    void SetValidationInterface(ValidationInterface<T>& validation_interface) const noexcept
    {
        kernel_context_options_set_validation_interface(m_options.get(), validation_interface.m_validation_interface);
    }

    friend class Context;
};

class Context
{
private:
    struct Deleter {
        void operator()(kernel_Context* ptr) const
        {
            kernel_context_destroy(ptr);
        }
    };

public:
    std::unique_ptr<kernel_Context, Deleter> m_context;

    Context(ContextOptions& opts) noexcept
        : m_context{kernel_context_create(opts.m_options.get())}
    {
    }

    Context() noexcept
        : m_context{kernel_context_create(ContextOptions{}.m_options.get())}
    {
    }

    /** Check whether this Context object is valid. */
    explicit operator bool() const noexcept { return bool{m_context}; }
};

class ChainstateManagerOptions
{
private:
    struct Deleter {
        void operator()(kernel_ChainstateManagerOptions* ptr) const
        {
            kernel_chainstate_manager_options_destroy(ptr);
        }
    };

    std::unique_ptr<kernel_ChainstateManagerOptions, Deleter> m_options;

public:
    ChainstateManagerOptions(const Context& context, const std::string& data_dir, const std::string& blocks_dir) noexcept
        : m_options{kernel_chainstate_manager_options_create(context.m_context.get(), data_dir.c_str(), data_dir.length(), blocks_dir.c_str(), blocks_dir.length())}
    {
    }

    void SetWorkerThreads(int worker_threads) const noexcept
    {
        kernel_chainstate_manager_options_set_worker_threads_num(m_options.get(), worker_threads);
    }

    bool SetWipeDbs(bool wipe_block_tree, bool wipe_chainstate) const noexcept
    {
        return kernel_chainstate_manager_options_set_wipe_dbs(m_options.get(), wipe_block_tree, wipe_chainstate);
    }

    void SetBlockTreeDbInMemory(bool block_tree_db_in_memory) const noexcept
    {
        kernel_chainstate_manager_options_set_block_tree_db_in_memory(m_options.get(), block_tree_db_in_memory);
    }

    void SetChainstateDbInMemory(bool chainstate_db_in_memory) const noexcept
    {
        kernel_chainstate_manager_options_set_chainstate_db_in_memory(m_options.get(), chainstate_db_in_memory);
    }

    /** Check whether this ChainstateManagerOptions object is valid. */
    explicit operator bool() const noexcept { return bool{m_options}; }

    friend class ChainMan;
};

class Block
{
private:
    struct Deleter {
        void operator()(kernel_Block* ptr) const
        {
            kernel_block_destroy(ptr);
        }
    };

    std::unique_ptr<kernel_Block, Deleter> m_block;

public:
    Block(const std::span<const unsigned char> raw_block) noexcept
        : m_block{kernel_block_create(raw_block.data(), raw_block.size())}
    {
    }

    /** Check whether this Block object is valid. */
    explicit operator bool() const noexcept { return bool{m_block}; }

    Block(kernel_Block* block) noexcept : m_block{block} {}

    std::unique_ptr<kernel_BlockHash, BlockHashDeleter> GetHash() const noexcept
    {
        return std::unique_ptr<kernel_BlockHash, BlockHashDeleter>(kernel_block_get_hash(m_block.get()));
    }

    std::vector<unsigned char> GetBlockData() const noexcept
    {
        auto serialized_block{kernel_copy_block_data(m_block.get())};
        std::vector<unsigned char> vec{serialized_block->data, serialized_block->data + serialized_block->size};
        kernel_byte_array_destroy(serialized_block);
        return vec;
    }

    friend class ChainMan;
};

class BlockUndo
{
private:
    struct Deleter {
        void operator()(kernel_BlockUndo* ptr) const
        {
            kernel_block_undo_destroy(ptr);
        }
    };

    const std::unique_ptr<kernel_BlockUndo, Deleter> m_block_undo;

public:
    const uint64_t m_size;

    BlockUndo(kernel_BlockUndo* block_undo) noexcept
        : m_block_undo{block_undo},
          m_size{kernel_block_undo_size(block_undo)}
    {
    }

    BlockUndo(const BlockUndo&) = delete;
    BlockUndo& operator=(const BlockUndo&) = delete;

    uint64_t GetTxOutSize(uint64_t index) const noexcept
    {
        return kernel_get_transaction_undo_size(m_block_undo.get(), index);
    }

    TransactionOutput GetTxUndoPrevoutByIndex(
        uint64_t tx_undo_index,
        uint64_t tx_prevout_index) const noexcept
    {
        return TransactionOutput{kernel_get_undo_output_by_index(m_block_undo.get(), tx_undo_index, tx_prevout_index)};
    }
};

class BlockIndex
{
private:
    struct Deleter {
        void operator()(kernel_BlockIndex* ptr) const
        {
            kernel_block_index_destroy(ptr);
        }
    };

    std::unique_ptr<kernel_BlockIndex, Deleter> m_block_index;

public:
    BlockIndex(kernel_BlockIndex* block_index) noexcept : m_block_index{block_index} {}

    std::optional<BlockIndex> GetPreviousBlockIndex() const noexcept
    {
        if (!m_block_index) {
            return std::nullopt;
        }
        auto index{kernel_get_previous_block_index(m_block_index.get())};
        if (!index) return std::nullopt;
        return index;
    }

    int32_t GetHeight() const noexcept
    {
        if (!m_block_index) {
            return -1;
        }
        return kernel_block_index_get_height(m_block_index.get());
    }

    std::unique_ptr<kernel_BlockHash, BlockHashDeleter> GetHash() const noexcept
    {
        if (!m_block_index) {
            return nullptr;
        }
        return std::unique_ptr<kernel_BlockHash, BlockHashDeleter>(kernel_block_index_get_block_hash(m_block_index.get()));
    }

    operator bool() const noexcept
    {
        return m_block_index && m_block_index.get();
    }

    friend class ChainMan;
};

class ChainMan
{
private:
    kernel_ChainstateManager* m_chainman;
    const Context& m_context;

public:
    ChainMan(const Context& context, const ChainstateManagerOptions& chainman_opts) noexcept
        : m_chainman{kernel_chainstate_manager_create(context.m_context.get(), chainman_opts.m_options.get())},
          m_context{context}
    {
    }

    /** Check whether this ChainMan object is valid. */
    explicit operator bool() const noexcept { return m_chainman != nullptr; }

    ChainMan(const ChainMan&) = delete;
    ChainMan& operator=(const ChainMan&) = delete;

    bool ImportBlocks(const std::span<const std::string> paths) const noexcept
    {
        std::vector<const char*> c_paths;
        std::vector<size_t> c_paths_lens;
        c_paths.reserve(paths.size());
        c_paths_lens.reserve(paths.size());
        for (const auto& path : paths) {
            c_paths.push_back(path.c_str());
            c_paths_lens.push_back(path.length());
        }

        return kernel_import_blocks(m_context.m_context.get(), m_chainman, c_paths.data(), c_paths_lens.data(), c_paths.size());
    }

    bool ProcessBlock(const Block& block, bool* new_block) const noexcept
    {
        return kernel_chainstate_manager_process_block(m_context.m_context.get(), m_chainman, block.m_block.get(), new_block);
    }

    BlockIndex GetBlockIndexFromTip() const noexcept
    {
        return kernel_get_block_index_from_tip(m_context.m_context.get(), m_chainman);
    }

    BlockIndex GetBlockIndexFromGenesis() const noexcept
    {
        return kernel_get_block_index_from_genesis(m_context.m_context.get(), m_chainman);
    }

    BlockIndex GetBlockIndexByHash(kernel_BlockHash* block_hash) const noexcept
    {
        return kernel_get_block_index_from_hash(m_context.m_context.get(), m_chainman, block_hash);
    }

    std::optional<BlockIndex> GetBlockIndexByHeight(int height) const noexcept
    {
        auto index{kernel_get_block_index_from_height(m_context.m_context.get(), m_chainman, height)};
        if (!index) return std::nullopt;
        return index;
    }

    std::optional<BlockIndex> GetNextBlockIndex(BlockIndex& block_index) const noexcept
    {
        auto index{kernel_get_next_block_index(m_context.m_context.get(), m_chainman, block_index.m_block_index.get())};
        if (!index) return std::nullopt;
        return index;
    }

    std::optional<Block> ReadBlock(BlockIndex& block_index) const noexcept
    {
        auto block{kernel_read_block_from_disk(m_context.m_context.get(), m_chainman, block_index.m_block_index.get())};
        if (!block) return std::nullopt;
        return block;
    }

    std::optional<BlockUndo> ReadBlockUndo(const BlockIndex& block_index) const noexcept
    {
        auto undo{kernel_read_block_undo_from_disk(m_context.m_context.get(), m_chainman, block_index.m_block_index.get())};
        if (!undo) return std::nullopt;
        return undo;
    }

    ~ChainMan()
    {
        kernel_chainstate_manager_destroy(m_chainman, m_context.m_context.get());
    }
};

#endif // BITCOIN_KERNEL_BITCOINKERNEL_WRAPPER_H
