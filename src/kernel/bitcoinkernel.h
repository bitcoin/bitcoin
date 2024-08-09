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
 * When passing the kernel_Error argument to a function it may either be null or
 * initialized to an initial value. It is recommended to set kernel_ERROR_OK
 * before passing it to a function.
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
 * The processing of validation events is done through the user-defined task
 * runner. Only a single task runner may be defined at a time for a context. The
 * task runner drives the execution of events triggering validation interface
 * callbacks. Multiple validation interfaces can be registered with the context.
 * The kernel will create an event for each of the registered validation
 * interfaces through the task runner.
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
 * Opaque data structure for holding a validation event. The event can only be
 * processed by calling kernel_execute_event_and_destroy(..).
 */
typedef struct kernel_ValidationEvent kernel_ValidationEvent;

/**
 * Opaque data structure for holding the state of a block during validation.
 *
 * Contains information indicating whether validation was successful, and if not
 * which step during block validation failed.
 */
typedef struct kernel_BlockValidationState kernel_BlockValidationState;

/**
 * Opaque data structure for holding a task runner.
 *
 * The task runner processes validation events and forwards them to the
 * registered validation interface.
 */
typedef struct kernel_TaskRunner kernel_TaskRunner;

/**
 * Opaque data structure for holding a validation interface.
 *
 * The validation interface can be registered with the task runner of an
 * existing context. It holds callbacks that will be triggered by certain
 * validation events.
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
 * Function signatures for the task runner used to process validation events.
 */
typedef void (*kernel_TaskRunnerInsert)(void* user_data, kernel_ValidationEvent* event);
typedef void (*kernel_TaskRunnerFlush)(void* user_data);
typedef uint32_t (*kernel_TaskRunnerSize)(void* user_data);

/**
 * Function signatures for the validation interface.
 */
typedef void (*kernel_ValidationInterfaceBlockChecked)(void* user_data, const kernel_BlockPointer* block, const kernel_BlockValidationState* state);

/**
 * Available types of context options. Passed with a corresponding value to
 * kernel_context_options_set(..).
 */
typedef enum {
    kernel_CHAIN_PARAMETERS_OPTION = 0, //!< Set the chain parameters, value must be a valid pointer
                                        //!< to a kernel_ChainParameters struct.
    kernel_NOTIFICATIONS_OPTION,        //!< Set the kernel notifications, value must be a valid
                                        //!< pointer to a kernel_Notifications struct.
    kernel_TASK_RUNNER_OPTION,          //!< Set the task runner, value must be a valid pointer to a
                                        //!< kernel_TaskRunner struct.
} kernel_ContextOptionType;

/**
 * Available types of chainstate load options. Passed with a corresponding value
 * to kernel_chainstate_load_options_set(..).
 */
typedef enum {
    kernel_WIPE_BLOCK_TREE_DB_CHAINSTATE_LOAD_OPTION = 0,  //! Set the wipe block tree db option, default is false.
                                                           //! Should only be set in combination with wiping the chainstate db.
                                                           //! Will trigger a reindex once kernel_import_blocks is called.
    kernel_WIPE_CHAINSTATE_DB_CHAINSTATE_LOAD_OPTION,      //! Set the wipe chainstate option, default is false.
    kernel_BLOCK_TREE_DB_IN_MEMORY_CHAINSTATE_LOAD_OPTION, //! Set the block tree db in memory option, default is false.
    kernel_CHAINSTATE_DB_IN_MEMORY_CHAINSTATE_LOAD_OPTION, //! Set the coins db in memory option, default is false.
} kernel_ChainstateLoadOptionType;

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
 * Holds the task runner callbacks. The user data pointer may be used to point
 * to user-defined structures to make processing the tasks easier. May also be
 * used to process the tasks asynchronously, or in separate threads.
 */
typedef struct {
    void* user_data;                //!< Holds a user-defined opaque structure that is passed to the task runner callbacks.
    kernel_TaskRunnerInsert insert; //!< Adds an event to the task runner. The event needs to be consumed with
                                    //!< kernel_execute_event_and_destroy, but may be queued for later, asynchronous or threaded
                                    //!< processing, or be executed immediately.
    kernel_TaskRunnerFlush flush;   //!< If this is called the user should ensure that all queued, or pending events
                                    //!< are executed before processing a further event.
    kernel_TaskRunnerSize size;     //!< Should return the number of queued or pending events that are awaiting
                                    //!< execution.
} kernel_TaskRunnerCallbacks;

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
    kernel_LOG_NONE = 0,
    kernel_LOG_ALL,
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
 * A collection of error codes that may be issued by the kernel library.
 */
typedef enum {
    kernel_ERROR_OK = 0,
    kernel_ERROR_TX_INDEX,
    kernel_ERROR_TX_SIZE_MISMATCH,
    kernel_ERROR_TX_DESERIALIZE,
    kernel_ERROR_INVALID_FLAGS,
    kernel_ERROR_INVALID_FLAGS_COMBINATION,
    kernel_ERROR_SPENT_OUTPUTS_REQUIRED,
    kernel_ERROR_SPENT_OUTPUTS_MISMATCH,
    kernel_ERROR_LOGGING_FAILED,
    kernel_ERROR_INVALID_CONTEXT,
    kernel_ERROR_INVALID_CONTEXT_OPTION,
    kernel_ERROR_INTERNAL,
    kernel_ERROR_DUPLICATE_BLOCK,
    kernel_ERROR_BLOCK_WITHOUT_COINBASE,
    kernel_ERROR_OUT_OF_BOUNDS,
} kernel_ErrorCode;

/**
 * Contains an error code and a pre-defined buffer for containing a string
 * describing a possible error.
 */
typedef struct {
    kernel_ErrorCode code;
    char message[256];
} kernel_Error;

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
 * A helper struct for a single transaction output.
 */
typedef struct {
    int64_t value;
    const unsigned char* script_pubkey;
    size_t script_pubkey_len;
} kernel_TransactionOutput;

/**
 * Chain type used for creating chain params.
 */
typedef enum {
    kernel_CHAIN_TYPE_MAINNET = 0,
    kernel_CHAIN_TYPE_TESTNET,
    kernel_CHAIN_TYPE_SIGNET,
    kernel_CHAIN_TYPE_REGTEST,
} kernel_ChainType;

/**
 * A type-safe block identifier.
 */
typedef struct {
    unsigned char hash[32];
} kernel_BlockHash;

/**
 * Holds some info about an indexed block.
 */
typedef struct {
    int height; //!< Non-zero if the block is in the chain. Once retrieved not guaranteed to remain accurate.
} kernel_BlockIndexInfo;

/**
 * Convenience struct for holding serialized data.
 */
typedef struct {
    unsigned char* data;
    size_t size;
} kernel_ByteArray;

/**
 * @brief Verify if the input at input_index of tx_to spends the script pubkey
 * under the constraints specified by flags. If the witness flag is set the
 * amount parameter is used. If the taproot flag is set, the spent outputs
 * parameter is used to validate taproot transactions.
 *
 * @param[in] script_pubkey     Non-null, serialized script pubkey to be spent.
 * @param[in] script_pubkey_len Length of the script pubkey to be spent.
 * @param[in] amount            Amount of the script pubkey's associated output. May be zero if
 *                              the witness flag is not set.
 * @param[in] tx_to             Non-null, serialized transaction spending the script_pubkey.
 * @param[in] tx_to_len         Length of the serialized transaction spending the script_pubkey.
 * @param[in] spent_outputs     Nullable if the taproot flag is not set. Points to an array of
 *                              outputs spent by the transaction.
 * @param[in] spent_outputs_len Length of the spent_outputs array.
 * @param[in] input_index       Index of the input in tx_to spending the script_pubkey.
 * @param[in] flags             Bitfield of kernel_ScriptFlags controlling validation constraints.
 * @param[out] error            Nullable, will contain an error/success code for the operation.
 * @return                      1 if the script is valid.
 */
int BITCOINKERNEL_WARN_UNUSED_RESULT kernel_verify_script(
    const unsigned char* script_pubkey, size_t script_pubkey_len,
    int64_t amount,
    const unsigned char* tx_to, size_t tx_to_len,
    const kernel_TransactionOutput* spent_outputs, size_t spent_outputs_len,
    unsigned int input_index,
    unsigned int flags,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(4);

/**
 * @brief This disables the global internal logger. No log messages will be
 * buffered internally anymore once this is called and the buffer is cleared.
 * This function should only be called once. Log messages will be buffered until
 * this function is called, or a logging connection is created.
 */
void kernel_disable_logging();

/**
 * @brief Set the log level of the global internal logger.
 *
 * @param[in] category If kernel_LOG_ALL is chosen, all messages at the specified level
 *                     will be logged. Otherwise only messages from the specified category
 *                     will be logged at the specified level and above.
 * @param[in] level    Log level at which the log category is set.
 */
void kernel_add_log_level_category(const kernel_LogCategory category, kernel_LogLevel level);

/**
 * Enable a specific log category for the global internal logger.
 */
void kernel_enable_log_category(const kernel_LogCategory category);

/**
 * Disable a specific log category for the global internal logger.
 */
void kernel_disable_log_category(const kernel_LogCategory category);

/**
 * @brief Start logging messages through the provided callback. Log messages
 * produced before this function is first called are buffered and on calling this
 * function are logged immediately.
 *
 * @param[in] callback  Non-null, function through which messages will be logged.
 * @param[in] user_data Nullable, holds a user-defined opaque structure. Is passed back
 *                      to the user through the callback.
 * @param[in] options   Sets formatting options of the log messages.
 * @param[out] error    Nullable, will contain an error/success code for the operation.
 */
kernel_LoggingConnection* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_logging_connection_create(
    kernel_LogCallback callback,
    void* user_data,
    const kernel_LoggingOptions options,
    kernel_Error* error
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
 * @brief Sets a single, specific field in the options. The option type has to
 * match the option value.
 *
 * @param[in] context_options Non-null, previously created with kernel_context_options_create.
 * @param[in] n_option        Describes the option field that should be set with the value.
 * @param[in] value           Non-null, single value that will be used to set the field selected by n_option.
 * @param[out] error          Nullable, error will contain an error/success code for the operation.
 */
void kernel_context_options_set(
    kernel_ContextOptions* context_options,
    const kernel_ContextOptionType n_option,
    const void* value,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(3);

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
 * @param[out] error          Nullable, will contain an error/success code for the operation.
 * @return                    The allocated kernel context, or null on error.
 */
kernel_Context* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_context_create(
    const kernel_ContextOptions* context_options,
    kernel_Error* error);

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
 * @param[out] error         Nullable, will contain an error/success code for the operation.
 * @return                   The allocated chainstate manager options, or null on error.
 */
kernel_ChainstateManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_options_create(
    const kernel_Context* context,
    const char* data_directory,
    kernel_Error* error
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
 * @param[out] error           Nullable, will contain an error/success code for the operation.
 * @return                     The allocated block manager options, or null on error.
 */
kernel_BlockManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_block_manager_options_create(
    const kernel_Context* context,
    const char* blocks_directory,
    kernel_Error* error
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
 * @param[out] error                     Nullable, will contain an error/success code for the operation.
 * @return                               The allocated chainstate manager, or null on error.
 */
kernel_ChainstateManager* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_create(
    kernel_ChainstateManagerOptions* chainstate_manager_options,
    kernel_BlockManagerOptions* block_manager_options,
    const kernel_Context* context,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * Destroy the chainstate manager.
 */
void kernel_chainstate_manager_destroy(kernel_ChainstateManager* chainstate_manager, const kernel_Context* context);

/**
 * @brief Create a task runner. The returned object can be used to set the
 * context options.
 *
 * @param[in] task_runner_callbacks The callbacks used for dispatching and managing events in the
 *                                  task runner.
 */
kernel_TaskRunner* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_task_runner_create(
    kernel_TaskRunnerCallbacks task_runner_callbacks);

/**
 * Destroy the task runner.
 */
void kernel_task_runner_destroy(kernel_TaskRunner* task_runner);

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
 * @param[out] error               Nullable, will contain an error/success code for the operation.
 */
void kernel_validation_interface_register(
    kernel_Context* context,
    kernel_ValidationInterface* validation_interface,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Unregister a validation interface from the internal task runner
 * associated with this context. This should be done before destroying the
 * kernel context it was previously registered with.
 *
 * @param[in] context              Non-null, will deregister the validation interface from this context.
 * @param[in] validation_interface Non-null.
 * @param[out] error               Nullable, will contain an error/success code for the operation.
 */
void kernel_validation_interface_unregister(
    kernel_Context* context,
    kernel_ValidationInterface* validation_interface,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * Destroy the validation interface. This should be done after unregistering it
 * if the validation interface was previously registered with a chainstate
 * manager.
 */
void kernel_validation_interface_destroy(kernel_ValidationInterface* validation_interface);

/**
 * @brief Use this (and only this) function to run the callback received through
 * the task runner's insert function. Once executed, the event is no longer
 * valid.
 *
 * @param[in] event  Non-null.
 * @param[out] error Nullable, will contain an error/success code for the operation.
 */
void kernel_execute_event_and_destroy(
    kernel_ValidationEvent* event,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Create options for loading the chainstate.
 */
kernel_ChainstateLoadOptions* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_load_options_create();

/**
 * @brief Sets a single, specific field in the chainstate load options. The
 * option type has to match the option value.
 *
 * @param[in] chainstate_load_options Non-null, created with kernel_chainstate_load_options_create.
 * @param[in] n_option                Describes the option field that should be set with the value.
 * @param[in] value                   Single value setting the field selected by n_option.
 */
void kernel_chainstate_load_options_set(
    kernel_ChainstateLoadOptions* chainstate_load_options,
    kernel_ChainstateLoadOptionType n_option,
    bool value
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
 * @param[out] error                  Nullable, will contain an error/success code for the operation.
 */
void kernel_chainstate_manager_load_chainstate(
    const kernel_Context* context,
    kernel_ChainstateLoadOptions* chainstate_load_options,
    kernel_ChainstateManager* chainstate_manager,
    kernel_Error* error
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
 * @param[out] error               Nullable, will contain an error/success code for the operation.
 */
void kernel_import_blocks(const kernel_Context* context,
                          kernel_ChainstateManager* chainstate_manager,
                          const char** block_file_paths, size_t block_file_paths_len,
                          kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Process and validate the passed in block with the chainstate manager.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block              Non-null, block to be validated.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       True if processing the block was successful.
 */
bool BITCOINKERNEL_WARN_UNUSED_RESULT kernel_chainstate_manager_process_block(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_Block* block,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Parse a serialized raw block into a new block object.
 *
 * @param[in] raw_block     Non-null, serialized block.
 * @param[in] raw_block_len Length of the serialized block.
 * @param[out] error        Nullable, will contain an error/success code for the operation.
 * @return                  The allocated block, or null on error.
 */
kernel_Block* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_block_create(
    const unsigned char* raw_block, size_t raw_block_len,
    kernel_Error* error
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
 * @brief Get the block index entry of the genesis block.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @return                       The block index of the genesis block, or null on error.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_block_index_from_genesis(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Retrieve a block index by its block hash.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_hash         Non-null.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       The block index of the block with the passed in hash, or null on error.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_block_index_by_hash(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_BlockHash* block_hash,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Retrieve a block index by its height in the currently active chain.
 * Once retrieved there is no guarantee that it remains in the active chain.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_height       Height in the chain of the to be retrieved block index.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       The block index at a certain height in the currently active chain, or null on error.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_block_index_by_height(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    int block_height,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2);

/**
 * @brief Return the next block index in the currently active chain, or null if
 * the current block index is the tip, or is not in the currently active
 * chain.
 *
 * @param[in] context            Non-null.
 * @param[in] block_index        Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       The next block index in the currently active chain, or null on error.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_next_block_index(
    const kernel_Context* context,
    kernel_BlockIndex* block_index,
    kernel_ChainstateManager* chainstate_manager,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Returns the previous block index in the chain, or null if the current
 * block index entry is the genesis block.
 *
 * @param[in] block_index Non-null.
 * @param[out] error      Nullable, will contain an error/success code for the operation.
 * @return                The previous block index, or null on error or if the current block index is the genesis block.
 */
kernel_BlockIndex* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_previous_block_index(
    kernel_BlockIndex* block_index,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Reads the block the passed in block index points to from disk and
 * returns it.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_index        Non-null.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       The read out block, or null on error.
 */
kernel_Block* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_read_block_from_disk(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_BlockIndex* block_index,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1) BITCOINKERNEL_ARG_NONNULL(2) BITCOINKERNEL_ARG_NONNULL(3);

/**
 * @brief Reads the block undo data the passed in block index points to from
 * disk and returns it.
 *
 * @param[in] context            Non-null.
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_index        Non-null.
 * @param[out] error             Nullable, will contain an error/success code for the operation.
 * @return                       The read out block undo data, or null on error.
 */
kernel_BlockUndo* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_read_block_undo_from_disk(
    const kernel_Context* context,
    kernel_ChainstateManager* chainstate_manager,
    kernel_BlockIndex* block_index,
    kernel_Error* error
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
 * @param[out] error                 Nullable, will contain an error/success code for the operation.
 * @return                           A transaction output pointer, or null on error.
 */
kernel_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_undo_output_by_index(
    kernel_BlockUndo* block_undo,
    uint64_t transaction_undo_index,
    uint64_t output_index,
    kernel_Error* error
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Return a block index info struct describing the passed in block index.
 *
 * @param[in] block_index Non-null.
 * @return                A block index info pointer, or null on error.
 */
kernel_BlockIndexInfo* BITCOINKERNEL_WARN_UNUSED_RESULT kernel_get_block_index_info(
    kernel_BlockIndex* block_index
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the block index info.
 */
void kernel_block_index_info_destroy(kernel_BlockIndexInfo* info);

/**
 * Destroy a kernel transaction output.
 */
void kernel_transaction_output_destroy(kernel_TransactionOutput* transaction_output);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
