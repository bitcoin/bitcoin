// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_H
#define BITCOIN_KERNEL_BITCOINKERNEL_H

#ifndef __cplusplus
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#endif // __cplusplus


#if !defined(BITCOINKERNEL_GNUC_PREREQ)
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define BITCOINKERNEL_GNUC_PREREQ(_maj, _min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((_maj) << 16) + (_min))
#else
#define BITCOINKERNEL_GNUC_PREREQ(_maj, _min) 0
#endif
#endif

/* Warning attributes */
#if defined(__GNUC__) && BITCOINKERNEL_GNUC_PREREQ(3, 4)
#define BITCOINKERNEL_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
#define BITCOINKERNEL_WARN_UNUSED_RESULT
#endif
#if !defined(BITCOINKERNEL_BUILD) && defined(__GNUC__) && BITCOINKERNEL_GNUC_PREREQ(3, 4)
#define BITCOINKERNEL_ARG_NONNULL(_x) __attribute__((__nonnull__(_x)))
#else
#define BITCOINKERNEL_ARG_NONNULL(_x)
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * ------ Context ------
 *
 * The library provides a built-in static constant kernel context. This context
 * offers only limited functionality. It detects and self-checks the correct
 * sha256 implementation, initializes the random number generator and
 * self-checks the secp256k1 static context. It is used internally for otherwise
 * "context-free" operations.
 *
 * The user can create their own context for passing it to state-rich validation
 * functions and holding callbacks for kernel events.
 *
 * ------ Error handling ------
 *
 * Functions communicate an error through their return types, usually returning
 * a nullptr, or false if an error is encountered. Additionally, verification
 * functions, e.g. for scripts, may communicate more detailed error information
 * through status code out parameters.
 *
 * Fine-grained validation information is communicated through the validation
 * interface.
 *
 * The kernel notifications issue callbacks for errors. These are usually
 * indicative of a system error. If such an error is issued, it is recommended
 * to halt and tear down the existing kernel objects. Remediating the error may
 * require system intervention by the user.
 *
 * ------ Pointer and argument conventions ------
 *
 * The user is responsible for de-allocating the memory owned by pointers
 * returned by functions. Typically pointers returned by *_create(...) functions
 * can be de-allocated by corresponding *_destroy(...) functions.
 *
 * Pointer arguments make no assumptions on their lifetime. Once the function
 * returns the user can safely de-allocate the passed in arguments.
 *
 * Pointers passed by callbacks are not owned by the user and are only valid for
 * the duration of it. They should not be de-allocated by the user.
 *
 * Array lengths follow the pointer argument they describe.
 */

/**
 * Opaque data structure for holding a transaction.
 */
typedef struct kernel_Transaction kernel_Transaction;

/**
 * Opaque data structure for holding a script pubkey.
 */
typedef struct kernel_ScriptPubkey kernel_ScriptPubkey;

/**
 * Opaque data structure for holding a transaction output.
 */
typedef struct kernel_TransactionOutput kernel_TransactionOutput;

/**
 * Opaque data structure for holding a logging connection.
 *
 * The logging connection can be used to manually stop logging.
 *
 * Messages that were logged before a connection is created are buffered in a
 * 1MB buffer. Logging can alternatively be permanently disabled by calling
 * kernel_disable_logging().
 */
typedef struct kernel_LoggingConnection kernel_LoggingConnection;

/**
 * Opaque data structure for holding the chain parameters.
 *
 * These are eventually placed into a kernel context through the kernel context
 * options. The parameters describe the properties of a chain, and may be
 * instantiated for either mainnet, testnet, signet, or regtest.
 */
typedef struct kernel_ChainParameters kernel_ChainParameters;

/**
 * Opaque data structure for holding callbacks for reacting to events that may
 * be encountered during library operations.
 */
typedef struct kernel_Notifications kernel_Notifications;

/**
 * Opaque data structure for holding options for creating a new kernel context.
 *
 * Once a kernel context has been created from these options, they may be
 * destroyed. The options hold the notification callbacks as well as the
 * selected chain type until they are passed to the context. Their content and
 * scope can be expanded over time.
 */
typedef struct kernel_ContextOptions kernel_ContextOptions;

/**
 * Opaque data structure for holding a kernel context.
 *
 * The kernel context is used to initialize internal state and hold the chain
 * parameters and callbacks for handling error and validation events. Once other
 * validation objects are instantiated from it, the context needs to be kept in
 * memory for the duration of their lifetimes.
 *
 * The processing of validation events is done through an internal task
 * runner owned by the context. The task runner drives the execution of events
 * triggering validation interface callbacks. Multiple validation interfaces can
 * be registered with the context. The kernel will create an event for each of
 * the registered validation interfaces through the task runner.
 *
 * A constructed context can be safely used from multiple threads, but functions
 * taking it as a non-cost argument need exclusive access to it.
 */
typedef struct kernel_Context kernel_Context;

/**
 * Opaque data structure for holding a block index pointer.
 *
 * This is a pointer to an element in the block index currently in memory of the
 * chainstate manager. It is valid for the lifetime of the chainstate manager it
 * was retrieved from.
 */
typedef struct kernel_BlockIndex kernel_BlockIndex;

/**
 * Opaque data structure for holding options for creating a new chainstate
 * manager.
 *
 * The chainstate manager options are used to set some parameters for the
 * chainstate manager. For now it just holds default options.
 */
typedef struct kernel_ChainstateManagerOptions kernel_ChainstateManagerOptions;

/**
 * Opaque data structure for holding options for creating a new chainstate
 * manager.
 *
 * The chainstate manager has an internal block manager that takes its own set
 * of parameters. It is initialized with default options.
 */
typedef struct kernel_BlockManagerOptions kernel_BlockManagerOptions;

/**
 * Opaque data structure for holding a chainstate manager.
 *
 * The chainstate manager is the central object for doing validation tasks as
 * well as retrieving data from the chain. Internally it is a complex data
 * structure with diverse functionality.
 *
 * The chainstate manager is only valid for as long as the context with which it
 * was created remains in memory.
 *
 * Its functionality will be more and more exposed in the future.
 */
typedef struct kernel_ChainstateManager kernel_ChainstateManager;

/**
 * Opaque data structure for holding parameters used for loading the chainstate
 * of a chainstate manager.
 *
 * Is initialized with default parameters.
 */
typedef struct kernel_ChainstateLoadOptions kernel_ChainstateLoadOptions;

/**
 * Opaque data structure for holding a block.
 */
typedef struct kernel_Block kernel_Block;

/**
 * Opaque data structure for holding a non-owned block. This is typically a
 * block available to the user through one of the validation callbacks.
 */
typedef struct kernel_BlockPointer kernel_BlockPointer;

/**
 * Opaque data structure for holding the state of a block during validation.
 *
 * Contains information indicating whether validation was successful, and if not
 * which step during block validation failed.
 */
typedef struct kernel_BlockValidationState kernel_BlockValidationState;

/**
 * Opaque data structure for holding a validation interface.
 *
 * The validation interface can be registered with the internal task runner of an
 * existing context. It holds callbacks that will be triggered by certain
 * validation events. The callbacks registered with it will block further
 * validation progress when triggered.
 */
typedef struct kernel_ValidationInterface kernel_ValidationInterface;

/**
 * Opaque data structure for holding a block undo struct.
 *
 * It holds all the previous outputs consumed by all transactions in a specific
 * block. Internally it holds a nested vector. The top level vector has an entry
 * for each transaction in a block (in order of the actual transactions of the
 * block and minus the coinbase transaction). Each entry is in turn a vector of
 * all the previous outputs of a transaction (in order of their corresponding
 * inputs).
 */
typedef struct kernel_BlockUndo kernel_BlockUndo;

/** Current sync state passed to tip changed callbacks. */
typedef enum {
    kernel_INIT_REINDEX,
    kernel_INIT_DOWNLOAD,
    kernel_POST_INIT
} kernel_SynchronizationState;

/** Possible warning types issued by validation. */
typedef enum {
    kernel_UNKNOWN_NEW_RULES_ACTIVATED,
    kernel_LARGE_WORK_INVALID_CHAIN
} kernel_Warning;

/** Callback function types */

/**
 * Function signature for the global logging callback. All bitcoin kernel
 * internal logs will pass through this callback.
 */
typedef void (*kernel_LogCallback)(void* user_data, const char* message);

/**
 * Function signatures for the kernel notifications.
 */
typedef void (*kernel_NotifyBlockTip)(void* user_data, kernel_SynchronizationState state, kernel_BlockIndex* index);
typedef void (*kernel_NotifyHeaderTip)(void* user_data, kernel_SynchronizationState state, int64_t height, int64_t timestamp, bool presync);
typedef void (*kernel_NotifyProgress)(void* user_data, const char* title, int progress_percent, bool resume_possible);
typedef void (*kernel_NotifyWarningSet)(void* user_data, kernel_Warning warning, const char* message);
typedef void (*kernel_NotifyWarningUnset)(void* user_data, kernel_Warning warning);
typedef void (*kernel_NotifyFlushError)(void* user_data, const char* message);
typedef void (*kernel_NotifyFatalError)(void* user_data, const char* message);

/**
 * Function signatures for the validation interface.
 */
typedef void (*kernel_ValidationInterfaceBlockChecked)(void* user_data, const kernel_BlockPointer* block, const kernel_BlockValidationState* state);

/**
 * Whether a validated data structure is valid, invalid, or an error was
 * encountered during processing.
 */
typedef enum {
    kernel_VALIDATION_STATE_VALID = 0,
    kernel_VALIDATION_STATE_INVALID,
    kernel_VALIDATION_STATE_ERROR,
} kernel_ValidationMode;

/**
 * A granular "reason" why a block was invalid.
 */
typedef enum {
    kernel_BLOCK_RESULT_UNSET = 0, //!< initial value. Block has not yet been rejected
    kernel_BLOCK_CONSENSUS,        //!< invalid by consensus rules (excluding any below reasons)
    /**
     * Invalid by a change to consensus rules more recent than SegWit.
     * Currently unused as there are no such consensus rule changes, and any download
     * sources realistically need to support SegWit in order to provide useful data,
     * so differentiating between always-invalid and invalid-by-pre-SegWit-soft-fork
     * is uninteresting.
     */
    kernel_BLOCK_RECENT_CONSENSUS_CHANGE,
    kernel_BLOCK_CACHED_INVALID,  //!< this block was cached as being invalid and we didn't store the reason why
    kernel_BLOCK_INVALID_HEADER,  //!< invalid proof of work or time too old
    kernel_BLOCK_MUTATED,         //!< the block's data didn't match the data committed to by the PoW
    kernel_BLOCK_MISSING_PREV,    //!< We don't have the previous block the checked one is built on
    kernel_BLOCK_INVALID_PREV,    //!< A block this one builds on is invalid
    kernel_BLOCK_TIME_FUTURE,     //!< block timestamp was > 2 hours in the future (or our clock is bad)
    kernel_BLOCK_CHECKPOINT,      //!< the block failed to meet one of our checkpoints
    kernel_BLOCK_HEADER_LOW_WORK, //!< the block header may be on a too-little-work chain
} kernel_BlockValidationResult;

/**
 * Holds the validation interface callbacks. The user data pointer may be used
 * to point to user-defined structures to make processing the validation
 * callbacks easier.
 */
typedef struct {
    void* user_data;                                      //!< Holds a user-defined opaque structure that is passed to the validation
                                                          //!< interface callbacks.
    kernel_ValidationInterfaceBlockChecked block_checked; //!< Called when a new block has been checked. Contains the
                                                          //!< result of its validation.
} kernel_ValidationInterfaceCallbacks;

/**
 * A struct for holding the kernel notification callbacks. The user data pointer
 * may be used to point to user-defined structures to make processing the
 * notifications easier.
 */
typedef struct {
    void* user_data;                         //!< Holds a user-defined opaque structure that is passed to the notification callbacks.
    kernel_NotifyBlockTip block_tip;         //!< The chain's tip was updated to the provided block index.
    kernel_NotifyHeaderTip header_tip;       //!< A new best block header was added.
    kernel_NotifyProgress progress;          //!< Reports on current block synchronization progress.
    kernel_NotifyWarningSet warning_set;     //!< A warning issued by the kernel library during validation.
    kernel_NotifyWarningUnset warning_unset; //!< A previous condition leading to the issuance of a warning is no longer given.
    kernel_NotifyFlushError flush_error;     //!< An error encountered when flushing data to disk.
    kernel_NotifyFatalError fatal_error;     //!< A un-recoverable system error encountered by the library.
} kernel_NotificationInterfaceCallbacks;

/**
 * A collection of logging categories that may be encountered by kernel code.
 */
typedef enum {
    kernel_LOG_ALL = 0,
    kernel_LOG_BENCH,
    kernel_LOG_BLOCKSTORAGE,
    kernel_LOG_COINDB,
    kernel_LOG_LEVELDB,
    kernel_LOG_LOCK,
    kernel_LOG_MEMPOOL,
    kernel_LOG_PRUNE,
    kernel_LOG_RAND,
    kernel_LOG_REINDEX,
    kernel_LOG_VALIDATION,
    kernel_LOG_KERNEL,
} kernel_LogCategory;

/**
 * The level at which logs should be produced.
 */
typedef enum {
    kernel_LOG_INFO = 0,
    kernel_LOG_DEBUG,
    kernel_LOG_TRACE,
} kernel_LogLevel;

/**
 * Options controlling the format of log messages.
 */
typedef struct {
    bool log_timestamps;               //!< Prepend a timestamp to log messages.
    bool log_time_micros;              //!< Log timestamps in microsecond precision.
    bool log_threadnames;              //!< Prepend the name of the thread to log messages.
    bool log_sourcelocations;          //!< Prepend the source location to log messages.
    bool always_print_category_levels; //!< Prepend the log category and level to log messages.
} kernel_LoggingOptions;

/**
 * A collection of status codes that may be issued by the script verify function.
 */
typedef enum {
    kernel_SCRIPT_VERIFY_OK = 0,
    kernel_SCRIPT_VERIFY_ERROR_TX_INPUT_INDEX, //!< The provided input index is out of range of the actual number of inputs of the transaction.
    kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS, //!< The provided bitfield for the flags was invalid.
    kernel_SCRIPT_VERIFY_ERROR_INVALID_FLAGS_COMBINATION, //!< The flags very combined in an invalid way.
    kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_REQUIRED, //!< The taproot flag was set, so valid spent_outputs have to be provided.
    kernel_SCRIPT_VERIFY_ERROR_SPENT_OUTPUTS_MISMATCH, //!< The number of spent outputs does not match the number of inputs of the tx.
} kernel_ScriptVerifyStatus;

/**
 * Script verification flags that may be composed with each other.
 */
typedef enum
{
    kernel_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    kernel_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), //!< evaluate P2SH (BIP16) subscripts
    kernel_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), //!< enforce strict DER (BIP66) compliance
    kernel_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), //!< enforce NULLDUMMY (BIP147)
    kernel_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), //!< enable CHECKLOCKTIMEVERIFY (BIP65)
    kernel_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), //!< enable CHECKSEQUENCEVERIFY (BIP112)
    kernel_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), //!< enable WITNESS (BIP141)

    kernel_SCRIPT_FLAGS_VERIFY_TAPROOT             = (1U << 17), //!< enable TAPROOT (BIPs 341 & 342)
    kernel_SCRIPT_FLAGS_VERIFY_ALL                 = kernel_SCRIPT_FLAGS_VERIFY_P2SH |
                                                     kernel_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                     kernel_SCRIPT_FLAGS_VERIFY_NULLDUMMY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY |
                                                     kernel_SCRIPT_FLAGS_VERIFY_WITNESS |
                                                     kernel_SCRIPT_FLAGS_VERIFY_TAPROOT
} kernel_ScriptFlags;

/**
 * Chain type used for creating chain params.
 */
typedef enum {
    kernel_CHAIN_TYPE_MAINNET = 0,
    kernel_CHAIN_TYPE_TESTNET,
    kernel_CHAIN_TYPE_TESTNET_4,
    kernel_CHAIN_TYPE_SIGNET,
    kernel_CHAIN_TYPE_REGTEST,
} kernel_ChainType;

/**
 * Process block statuses. More detailed information about block verification
 * can also be gathered through a registered validation interface.
 */
typedef enum {
    kernel_PROCESS_BLOCK_OK = 0,
    kernel_PROCESS_BLOCK_INVALID,           //!< The block failed processing.
    kernel_PROCESS_BLOCK_ERROR_NO_COINBASE, //!< To process a block, a coinbase transaction has to be part of it.
    kernel_PROCESS_BLOCK_DUPLICATE,         //!< The block has been processed before.
    kernel_PROCESS_BLOCK_INVALID_DUPLICATE, //!< The block has been process before, and it was invalid.
} kernel_ProcessBlockStatus;

/**
 * Convenience struct for holding serialized data.
 */
typedef struct {
    unsigned char* data;
    size_t size;
} kernel_ByteArray;

/**
 * @brief Create a new transaction from the serialized data.
 *
 * @param[in] raw_transaction     Non-null.
 * @param[in] raw_transaction_len Length of the serialized transaction.
 * @return                        The transaction, or null on error.
 */
kernel_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_transaction_create(
    const unsigned char* raw_transaction, size_t raw_transaction_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction.
 */
void kernel_transaction_destroy(kernel_Transaction* transaction);

/**
 * @brief Create a script pubkey from serialized data.
 * @param[in] script_pubkey     Non-null.
 * @param[in] script_pubkey_len Length of the script pubkey data.
 * @return                      The script pubkey, or null on error.
 */
kernel_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_script_pubkey_create(
    const unsigned char* script_pubkey, size_t script_pubkey_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copies the script pubkey data into the returned byte array.
 * @param[in] script_pubkey Non-null.
 * @return                  The serialized script pubkey data.
 */
kernel_ByteArray* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_copy_script_pubkey_data(
        const kernel_ScriptPubkey* script_pubkey
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the script pubkey.
 */
void kernel_script_pubkey_destroy(kernel_ScriptPubkey* script_pubkey);

/**
 * @brief Create a transaction output from a script pubkey and an amount.
 * @param[in] script_pubkey Non-null.
 * @param[in] amount        The amount associated with the script pubkey for this output.
 * @return                  The transaction output.
 */
kernel_TransactionOutput* kernel_transaction_output_create(
    kernel_ScriptPubkey* script_pubkey,
    int64_t amount
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction output.
 */
void kernel_transaction_output_destroy(kernel_TransactionOutput* transaction_output);

/**
 * @brief Verify if the input at input_index of tx_to spends the script pubkey
 * under the constraints specified by flags. If the witness flag is set the
 * amount parameter is used. If the taproot flag is set, the spent outputs
 * parameter is used to validate taproot transactions.
 *
 * @param[in] script_pubkey     Non-null, script pubkey to be spent.
 * @param[in] amount            Amount of the script pubkey's associated output. May be zero if
 *                              the witness flag is not set.
 * @param[in] tx_to             Non-null, transaction spending the script_pubkey.
 * @param[in] spent_outputs     Nullable if the taproot flag is not set. Points to an array of
 *                              outputs spent by the transaction.
 * @param[in] spent_outputs_len Length of the spent_outputs array.
 * @param[in] input_index       Index of the input in tx_to spending the script_pubkey.
 * @param[in] flags             Bitfield of kernel_ScriptFlags controlling validation constraints.
 * @param[out] status           Nullable, will be set to an error code if the operation fails.
 *                              Should be set to kernel_SCRIPT_VERIFY_OK.
 * @return                      True if the script is valid.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_verify_script(
    const kernel_ScriptPubkey* script_pubkey,
    int64_t amount,
    const kernel_Transaction* tx_to,
    const kernel_TransactionOutput** spent_outputs, size_t spent_outputs_len,
    unsigned int input_index,
    unsigned int flags,
    kernel_ScriptVerifyStatus* status
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief This disables the global internal logger. No log messages will be
 * buffered internally anymore once this is called and the buffer is cleared.
 * This function should only be called once. Log messages will be buffered until
 * this function is called, or a logging connection is created.
 */
void kernel_disable_logging();

/**
 * @brief Set the log level of the global internal logger. This does not enable
 * the selected categories. Use `kernel_enable_log_category` to start logging
 * from a specific, or all categories.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all messages at the specified level
 *                     will be logged. Otherwise only messages from the specified category
 *                     will be logged at the specified level and above.
 * @param[in] level    Log level at which the log category is set.
 * @return             True on success.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_add_log_level_category(const kernel_LogCategory category, kernel_LogLevel level);

/**
 * @brief Enable a specific log category for the global internal logger.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all categories will be enabled.
 * @return             True on success.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_enable_log_category(const kernel_LogCategory category);

/**
 * Disable a specific log category for the global internal logger.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all categories will be disabled.
 * @return             True on success.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_disable_log_category(const kernel_LogCategory category);

/**
 * @brief Start logging messages through the provided callback. Log messages
 * produced before this function is first called are buffered and on calling this
 * function are logged immediately.
 *
 * @param[in] callback  Non-null, function through which messages will be logged.
 * @param[in] user_data Nullable, holds a user-defined opaque structure. Is passed back
 *                      to the user through the callback.
 * @param[in] options   Sets formatting options of the log messages.
 * @return              A new kernel logging connection, or null on error.
 */
kernel_LoggingConnection* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_logging_connection_create(
    kernel_LogCallback callback,
    void* user_data,
    const kernel_LoggingOptions options
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Stop logging and destroy the logging connection.
 */
void kernel_logging_connection_destroy(kernel_LoggingConnection* logging_connection);

/**
 * @brief Creates a chain parameters struct with default parameters based on the
 * passed in chain type.
 *
 * @param[in] chain_type Controls the chain parameters type created.
 * @return               An allocated chain parameters opaque struct.
 */
const kernel_ChainParameters* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chain_parameters_create(
    const kernel_ChainType chain_type);

/**
 * Destroy the chain parameters.
 */
void kernel_chain_parameters_destroy(const kernel_ChainParameters* chain_parameters);

/**
 * @brief Creates an object for holding the kernel notification callbacks.
 *
 * @param[in] callbacks Holds the callbacks that will be invoked by the kernel notifications.
 */
kernel_Notifications* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_notifications_create(
    kernel_NotificationInterfaceCallbacks callbacks);

/**
 * Destroy the kernel notifications.
 */
void kernel_notifications_destroy(const kernel_Notifications* notifications);

/**
 * Creates an empty context options.
 */
kernel_ContextOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_options_create();

/**
 * @brief Sets the chain params for the context options. The context created
 * with the options will be configured for these chain parameters.
 *
 * @param[in] context_options  Non-null, previously created with kernel_context_options_create.
 * @param[in] chain_parameters Is set to the context options.
 */
void kernel_context_options_set_chainparams(
    kernel_ContextOptions* context_options,
    const kernel_ChainParameters* chain_parameters
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Set the kernel notifications for the context options. The context
 * created with the options will be configured with these notifications.
 *
 * @param[in] context_options Non-null, previously created with kernel_context_options_create.
 * @param[in] notifications   Is set to the context options.
 */
void kernel_context_options_set_notifications(
    kernel_ContextOptions* context_options,
    const kernel_Notifications* notifications
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * Destroy the context options.
 */
void kernel_context_options_destroy(kernel_ContextOptions* context_options);

/**
 * @brief Create a new kernel context. If the options have not been previously
 * set, their corresponding fields will be initialized to default values; the
 * context will assume mainnet chain parameters and won't attempt to call the
 * kernel notification callbacks.
 *
 * @param[in] context_options Nullable, created with kernel_context_options_create.
 * @return                    The allocated kernel context, or null on error.
 */
kernel_Context* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_create(
    const kernel_ContextOptions* context_options);

/**
 * @brief Interrupt can be used to halt long-running validation functions like
 * when reindexing, importing or processing blocks.
 *
 * @param[in] context  Non-null.
 * @return             True if the interrupt was successful.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_interrupt(
    kernel_Context* context
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the context.
 */
void kernel_context_destroy(kernel_Context* context);

/**
 * @brief Create options for the chainstate manager.
 *
 * @param[in] context        Non-null, the created options will associate with this kernel context
 *                           for the duration of their lifetime. The same context needs to be used
 *                           when instantiating the chainstate manager.
 * @param[in] data_directory Non-null, directory containing the chainstate data. If the directory
 *                           does not exist yet, it will be created.
 * @return                   The allocated chainstate manager options, or null on error.
 */
kernel_ChainstateManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_options_create(
    const kernel_Context* context,
    const char* data_directory
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * Destroy the chainstate manager options.
 */
void kernel_chainstate_manager_options_destroy(kernel_ChainstateManagerOptions* chainstate_manager_options);

/**
 * @brief Create options for the block manager. The block manager is used
 * internally by the chainstate manager for block storage and indexing.
 *
 * @param[in] context          Non-null, the created options will associate with this kernel context
 *                             for the duration of their lifetime. The same context needs to be used
 *                             when instantiating the chainstate manager.
 * @param[in] blocks_directory Non-null, directory containing the block data. If the directory does
 *                             not exist yet, it will be created.
 * @return                     The allocated block manager options, or null on error.
 */
kernel_BlockManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_block_manager_options_create(
    const kernel_Context* context,
    const char* blocks_directory
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * Destroy the block manager options.
 */
void kernel_block_manager_options_destroy(kernel_BlockManagerOptions* block_manager_options);

/**
 * @brief Create a chainstate manager. This is the main object for many
 * validation tasks as well as for retrieving data from the chain. It is only
 * valid for as long as the passed in context also remains in memory.
 *
 * @param[in] chainstate_manager_options Non-null, created by kernel_chainstate_manager_options_create.
 * @param[in] block_manager_options      Non-null, created by kernel_block_manager_options_create.
 * @param[in] context                    Non-null, the created chainstate manager will associate with this
 *                                       kernel context for the duration of its lifetime. The same context
 *                                       needs to be used for later interactions with the chainstate manager.
 * @return                               The allocated chainstate manager, or null on error.
 */
kernel_ChainstateManager* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_create(
    kernel_ChainstateManagerOptions* chainstate_manager_options,
    kernel_BlockManagerOptions* block_manager_options,
    const kernel_Context* context
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * Destroy the chainstate manager.
 */
void kernel_chainstate_manager_destroy(kernel_ChainstateManager* chainstate_manager, const kernel_Context* context);

/**
 * @brief Creates a new validation interface for consuming events issued by the
 * chainstate manager. The interface should be created and registered before the
 * chainstate manager is created to avoid missing validation events.
 *
 * @param[in] validation_interface_callbacks The callbacks used for passing validation information to the
 *                                           user.
 * @return                                   A validation interface. This should remain in memory for as
 *                                           long as the user expects to receive validation events.
 */
kernel_ValidationInterface* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_validation_interface_create(
    kernel_ValidationInterfaceCallbacks validation_interface_callbacks);

/**
 * @brief Register a validation interface with the internal task runner
 * associated with this context. This also registers it with the chainstate
 * manager if the chainstate manager is subsequently created with this context.
 *
 * @param[in] context              Non-null, will register the validation interface with this context.
 * @param[in] validation_interface Non-null.
 * @return                         True on success.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_validation_interface_register(
    kernel_Context* context,
    kernel_ValidationInterface* validation_interface
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Unregister a validation interface from the internal task runner
 * associated with this context. This should be done before destroying the
 * kernel context it was previously registered with.
 *
 * @param[in] context              Non-null, will deregister the validation interface from this context.
 * @param[in] validation_interface Non-null.
 * @return                         True on success.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_validation_interface_unregister(
    kernel_Context* context,
    kernel_ValidationInterface* validation_interface
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * Destroy the validation interface. This should be done after unregistering it
 * if the validation interface was previously registered with a chainstate
 * manager.
 */
void kernel_validation_interface_destroy(kernel_ValidationInterface* validation_interface);

/**
 * Create options for loading the chainstate.
 */
kernel_ChainstateLoadOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_load_options_create();

/**
 * @brief Sets wipe block tree db in the chainstate load options.
 *
 * @param[in] chainstate_load_options Non-null, created with kernel_chainstate_load_options_create.
 * @param[in] wipe_block_tree_db      Set wipe block tree db.
 */
void kernel_chainstate_load_options_set_wipe_block_tree_db(
    kernel_ChainstateLoadOptions* chainstate_load_options,
    bool wipe_block_tree_db
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets wipe chainstate db in the chainstate load options.
 *
 * @param[in] chainstate_load_options Non-null, created with kernel_chainstate_load_options_create.
 * @param[in] wipe_chainstate_db      Set wipe chainstate db.
 */
void kernel_chainstate_load_options_set_wipe_chainstate_db(
    kernel_ChainstateLoadOptions* chainstate_load_options,
    bool wipe_chainstate_db
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets block tree db in memory in the chainstate load options.
 *
 * @param[in] chainstate_load_options Non-null, created with kernel_chainstate_load_options_create.
 * @param[in] block_tree_db_in_memory Set block tree db in memory.
 */
void kernel_chainstate_load_options_set_block_tree_db_in_memory(
    kernel_ChainstateLoadOptions* chainstate_load_options,
    bool block_tree_db_in_memory
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets chainstate db in memory in the chainstate load options.
 *
 * @param[in] chainstate_load_options Non-null, created with kernel_chainstate_load_options_create.
 * @param[in] chainstate_db_in_memory Set chainstate db in memory.
 */
void kernel_chainstate_load_options_set_chainstate_db_in_memory(
    kernel_ChainstateLoadOptions* chainstate_load_options,
    bool chainstate_db_in_memory
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the chainstate load options
 */
void kernel_chainstate_load_options_destroy(kernel_ChainstateLoadOptions* chainstate_load_options);

/**
 * @brief This function must be called to initialize the chainstate manager
 * before doing validation tasks or interacting with its indexes.
 *
 * @param[in] context                 Non-null.
 * @param[in] chainstate_load_options Non-null, created by kernel_chainstate_load_options_create.
 * @param[in] chainstate_manager      Non-null, will load the chainstate(s) and initialize indexes.
 * @return                            True on success, false on error.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_load_chainstate(
    const kernel_Context* context,
    kernel_ChainstateLoadOptions* chainstate_load_options,
    kernel_ChainstateManager* chainstate_manager
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief May be called after kernel_chainstate_manager_load_chainstate to
 * initialize the chainstate manager. Triggers the start of a reindex if the
 * option was previously set for the chainstate and block manager. Can also
 * import an array of existing block files selected by the user.
 *
 * @param[in] context              Non-null.
 * @param[in] chainstate_manager   Non-null.
 * @param[in] block_file_paths     Nullable, array of block files described by their full filesystem paths.
 * @param[in] block_file_paths_len Length of the block_file_paths array.
 * @return                         True if the import blocks call was completed successfully.
 */
bool kernel_import_blocks(const kernel_Context* context,
                          kernel_ChainstateManager* chainstate_manager,
                          const char** block_file_paths, size_t block_file_paths_len
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Process and validate the passed in block with the chainstate manager.
 * If processing failed, some information can be retrieved through the status
 * enumeration. More detailed validation information in case of a failure can
 * also be retrieved through a registered validation interface. If the block
 * fails to validate the `block_checked` callback's 'BlockValidationState' will
 * contain details.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block              Non-null, block to be validated.
 * @param[out] status            Nullable, will contain an error/success code for the operation.
 * @return                       True if processing the block was successful.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_process_block(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_Block* block,
    kernel_ProcessBlockStatus* status
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Parse a serialized raw block into a new block object.
 *
 * @param[in] raw_block     Non-null, serialized block.
 * @param[in] raw_block_len Length of the serialized block.
 * @return                  The allocated block, or null on error.
 */
kernel_Block* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_block_create(
    const unsigned char* raw_block, size_t raw_block_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the block.
 */
void kernel_block_destroy(kernel_Block* block);

/**
 * @brief Copies block data into the returned byte array.
 *
 * @param[in] block  Non-null.
 * @return           Allocated byte array holding the block data, or null on error.
 */
kernel_ByteArray* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_copy_block_data(
    kernel_Block* block
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copies block data into the returned byte array.
 *
 * @param[in] block  Non-null.
 * @return           Allocated byte array holding the block data, or null on error.
 */
kernel_ByteArray* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_copy_block_pointer_data(
    const kernel_BlockPointer* block
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * A helper function for destroying an existing byte array.
 */
void kernel_byte_array_destroy(kernel_ByteArray* byte_array);

/**
 * Returns the validation mode from an opaque block validation state pointer.
 */
kernel_ValidationMode kernel_get_validation_mode_from_block_validation_state(
    const kernel_BlockValidationState* block_validation_state
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Returns the validation result from an opaque block validation state pointer.
 */
kernel_BlockValidationResult kernel_get_block_validation_result_from_block_validation_state(
    const kernel_BlockValidationState* block_validation_state
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the block index entry of the current chain tip. Once returned,
 * there is no guarantee that it remains in the active chain.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @return                       The block index of the current tip.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_block_index_from_tip(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Returns the previous block index in the chain, or null if the current
 * block index entry is the genesis block.
 *
 * @param[in] block_index Non-null.
 * @return                The previous block index, or null on error or if the current block index is the genesis block.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_previous_block_index(
    kernel_BlockIndex* block_index
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Reads the block the passed in block index points to from disk and
 * returns it.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_index        Non-null.
 * @return                       The read out block, or null on error.
 */
kernel_Block* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_read_block_from_disk(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_BlockIndex* block_index
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Reads the block undo data the passed in block index points to from
 * disk and returns it.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_index        Non-null.
 * @return                       The read out block undo data, or null on error.
 */
kernel_BlockUndo* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_read_block_undo_from_disk(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_BlockIndex* block_index
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Destroy the block index.
 */
void kernel_block_index_destroy(kernel_BlockIndex* block_index);

/**
 * @brief Returns the number of transactions whose undo data is contained in
 * block undo.
 *
 * @param[in] block_undo Non-null.
 * @return               The number of transaction undo data in the block undo.
 */
uint64_t BITCOINKERNEL_WARN_UNUSED_RESULT kernel_block_undo_size(
    kernel_BlockUndo* block_undo
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the block undo data.
 */
void kernel_block_undo_destroy(kernel_BlockUndo* block_undo);

/**
 * @brief Returns the number of previous transaction outputs contained in the
 * transaction undo data.
 *
 * @param[in] block_undo             Non-null, the block undo data from which tx_undo was retrieved from.
 * @param[in] transaction_undo_index The index of the transaction undo data within the block undo data.
 * @return                           The number of previous transaction outputs in the transaction.
 */
uint64_t BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_transaction_undo_size(
    kernel_BlockUndo* block_undo,
    uint64_t transaction_undo_index
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Return a transaction output contained in the transaction undo data of
 * a block undo data at a certain index.
 *
 * @param[in] block_undo             Non-null.
 * @param[in] transaction_undo_index The index of the transaction undo data within the block undo data.
 * @param[in] output_index           The index of the to be retrieved transaction output within the
 *                                   transaction undo data.
 * @return                           A transaction output pointer, or null on error.
 */
kernel_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_undo_output_by_index(
    kernel_BlockUndo* block_undo,
    uint64_t transaction_undo_index,
    uint64_t output_index
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copies the script pubkey of an output in the returned script pubkey
 * opaque object.
 *
 * @param[in] transaction_output Non-null.
 * @return                       The data for the output's script pubkey.
 */
kernel_ScriptPubkey* kernel_copy_script_pubkey_from_output(kernel_TransactionOutput* transaction_output
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Gets the amount associated with this transaction output
 *
 * @param[in] transaction_output Non-null.
 * @return                       The amount.
 */
int64_t kernel_get_transaction_output_amount(kernel_TransactionOutput* transaction_output
) BITCOINKERNEL_ARG_NONNULL(1);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
