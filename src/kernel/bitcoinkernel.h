// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_BITCOINKERNEL_H
#define BITCOIN_KERNEL_BITCOINKERNEL_H

#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#endif // __cplusplus

#ifndef BITCOINKERNEL_API
    #ifdef BITCOINKERNEL_BUILD
        #if defined(_WIN32)
            #define BITCOINKERNEL_API __declspec(dllexport)
        #elif !defined(_WIN32) && defined(__GNUC__)
            #define BITCOINKERNEL_API __attribute__((visibility("default")))
        #else
            #define BITCOINKERNEL_API
        #endif
    #else
        #if defined(_WIN32) && !defined(BITCOINKERNEL_STATIC)
            #define BITCOINKERNEL_API __declspec(dllimport)
        #else
            #define BITCOINKERNEL_API
        #endif
    #endif
#endif

/* Warning attributes */
#if defined(__GNUC__)
    #define BITCOINKERNEL_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
    #define BITCOINKERNEL_WARN_UNUSED_RESULT
#endif
#if !defined(BITCOINKERNEL_BUILD) && defined(__GNUC__)
    #define BITCOINKERNEL_ARG_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#else
    #define BITCOINKERNEL_ARG_NONNULL(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @page remarks Remarks
 *
 * @section purpose Purpose
 *
 * This header currently exposes an API for interacting with parts of Bitcoin
 * Core's consensus code. Users can validate blocks, iterate the block index,
 * read block und undo data from disk, and validate scripts. The header is
 * unversioned and not stable yet. Users should expect breaking changes. It is
 * also not yet included in releases of Bitcoin Core.
 *
 * @section context Context
 *
 * The library provides a built-in static constant kernel context. This static
 * context offers only limited functionality. It detects and self-checks the
 * correct sha256 implementation, initializes the random number generator and
 * self-checks the secp256k1 static context. It is used internally for
 * otherwise "context-free" operations. This means that the user is not
 * required to initialize their own context before using the library.
 *
 * The user should create their own context for passing it to state-rich validation
 * functions and holding callbacks for kernel events.
 *
 * @section error Error handling
 *
 * Functions communicate an error through their return types, usually returning
 * a nullptr, 0, or false if an error is encountered. Additionally, verification
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
 * @section pointer Pointer and argument conventions
 *
 * The user is responsible for de-allocating the memory owned by pointers
 * returned by functions. Typically pointers returned by *_create(...) functions
 * can be de-allocated by corresponding *_destroy(...) functions.
 *
 * A function that takes pointer arguments makes no assumptions on their
 * lifetime. Once the function returns the user can safely de-allocate the
 * passed in arguments.
 *
 * Pointers passed by callbacks are not owned by the user and are only valid
 * for the duration of the callback. They are always marked as `const` and must
 * not be de-allocated by the user.
 *
 * Array lengths follow the pointer argument they describe.
 */

/**
 * Opaque data structure for holding a transaction.
 */
typedef struct btck_Transaction btck_Transaction;

/**
 * Opaque data structure for holding a script pubkey.
 */
typedef struct btck_ScriptPubkey btck_ScriptPubkey;

/**
 * Opaque data structure for holding a transaction output.
 */
typedef struct btck_TransactionOutput btck_TransactionOutput;

/**
 * Opaque data structure for holding a logging connection.
 *
 * The logging connection can be used to manually stop logging.
 *
 * Messages that were logged before a connection is created are buffered in a
 * 1MB buffer. Logging can alternatively be permanently disabled by calling
 * @ref btck_logging_disable. Functions changing the logging settings are
 * global (and not thread safe) and change the settings for all existing
 * btck_LoggingConnection instances.
 */
typedef struct btck_LoggingConnection btck_LoggingConnection;

/**
 * Opaque data structure for holding the chain parameters.
 *
 * These are eventually placed into a kernel context through the kernel context
 * options. The parameters describe the properties of a chain, and may be
 * instantiated for either mainnet, testnet, signet, or regtest.
 */
typedef struct btck_ChainParameters btck_ChainParameters;

/**
 * Opaque data structure for holding options for creating a new kernel context.
 *
 * Once a kernel context has been created from these options, they may be
 * destroyed. The options hold the notification callbacks as well as the
 * selected chain type until they are passed to the context. If no options are
 * configured, the context will be instantiated with no callbacks and for
 * mainnet. Their content and scope can be expanded over time.
 */
typedef struct btck_ContextOptions btck_ContextOptions;

/**
 * Opaque data structure for holding a kernel context.
 *
 * The kernel context is used to initialize internal state and hold the chain
 * parameters and callbacks for handling error and validation events. Once other
 * validation objects are instantiated from it, the context is kept in memory
 * for the duration of their lifetimes.
 *
 * The processing of validation events is done through an internal task runner
 * owned by the context. It passes events through the registered validation
 * interface callbacks.
 *
 * A constructed context can be safely used from multiple threads.
 */
typedef struct btck_Context btck_Context;

/**
 * Opaque data structure for holding a block tree entry.
 *
 * This is a pointer to an element in the block index currently in memory of
 * the chainstate manager. It is valid for the lifetime of the chainstate
 * manager it was retrieved from. The entry is part of a tree-like structure
 * that is maintained internally. Every entry, besides the genesis, points to a
 * single parent. Multiple entries may share a parent, thus forming a tree.
 * Each entry corresponds to a single block and may be used to retrieve its
 * data and validation status.
 */
typedef struct btck_BlockTreeEntry btck_BlockTreeEntry;

/**
 * Opaque data structure for holding options for creating a new chainstate
 * manager.
 *
 * The chainstate manager options are used to set some parameters for the
 * chainstate manager. For now it just holds default options.
 */
typedef struct btck_ChainstateManagerOptions btck_ChainstateManagerOptions;

/**
 * Opaque data structure for holding a chainstate manager.
 *
 * The chainstate manager is the central object for doing validation tasks as
 * well as retrieving data from the chain. Internally it is a complex data
 * structure with diverse functionality.
 *
 * Its functionality will be more and more exposed in the future.
 */
typedef struct btck_ChainstateManager btck_ChainstateManager;

/**
 * Opaque data structure for holding a block.
 */
typedef struct btck_Block btck_Block;

/**
 * Opaque data structure for holding the state of a block during validation.
 *
 * Contains information indicating whether validation was successful, and if not
 * which step during block validation failed.
 */
typedef struct btck_BlockValidationState btck_BlockValidationState;

/**
 * Opaque data structure for holding the currently known best-chain associated
 * with a chainstate.
 */
typedef struct btck_Chain btck_Chain;

/**
 * Opaque data structure for holding a block's spent outputs.
 *
 * Contains all the previous outputs consumed by all transactions in a specific
 * block. Internally it holds a nested vector. The top level vector has an
 * entry for each transaction in a block (in order of the actual transactions
 * of the block and without the coinbase transaction). This is exposed through
 * @ref btck_TransactionSpentOutputs. Each btck_TransactionSpentOutputs is in
 * turn a vector of all the previous outputs of a transaction (in order of
 * their corresponding inputs).
 */
typedef struct btck_BlockSpentOutputs btck_BlockSpentOutputs;

/**
 * Opaque data structure for holding a transaction's spent outputs.
 *
 * Holds the coins consumed by a certain transaction. Retrieved through the
 * @ref btck_BlockSpentOutputs. The coins are in the same order as the
 * transaction's inputs consuming them.
 */
typedef struct btck_TransactionSpentOutputs btck_TransactionSpentOutputs;

/**
 * Opaque data structure for holding a coin.
 *
 * Holds information on the @ref btck_TransactionOutput held within,
 * including the height it was spent at and whether it is a coinbase output.
 */
typedef struct btck_Coin btck_Coin;

/** Current sync state passed to tip changed callbacks. */
typedef uint8_t btck_SynchronizationState;
#define btck_SynchronizationState_INIT_REINDEX ((btck_SynchronizationState)(0))
#define btck_SynchronizationState_INIT_DOWNLOAD ((btck_SynchronizationState)(1))
#define btck_SynchronizationState_POST_INIT ((btck_SynchronizationState)(2))

/** Possible warning types issued by validation. */
typedef uint8_t btck_Warning;
#define btck_Warning_UNKNOWN_NEW_RULES_ACTIVATED ((btck_Warning)(0))
#define btck_Warning_LARGE_WORK_INVALID_CHAIN ((btck_Warning)(1))

/** Callback function types */

/**
 * Function signature for the global logging callback. All bitcoin kernel
 * internal logs will pass through this callback.
 */
typedef void (*btck_LogCallback)(void* user_data, const char* message, size_t message_len);

/**
 * Function signature for freeing user data.
 */
typedef void (*btck_DestroyCallback)(void* user_data);

/**
* Function signatures for the kernel notifications.
 */
typedef void (*btck_NotifyBlockTip)(void* user_data, btck_SynchronizationState state, btck_BlockTreeEntry* entry, double verification_progress);
typedef void (*btck_NotifyHeaderTip)(void* user_data, btck_SynchronizationState state, int64_t height, int64_t timestamp, int presync);
typedef void (*btck_NotifyProgress)(void* user_data, const char* title, size_t title_len, int progress_percent, int resume_possible);
typedef void (*btck_NotifyWarningSet)(void* user_data, btck_Warning warning, const char* message, size_t message_len);
typedef void (*btck_NotifyWarningUnset)(void* user_data, btck_Warning warning);
typedef void (*btck_NotifyFlushError)(void* user_data, const char* message, size_t message_len);
typedef void (*btck_NotifyFatalError)(void* user_data, const char* message, size_t message_len);

/**
 * Function signatures for the validation interface.
 */
typedef void (*btck_ValidationInterfaceBlockChecked)(void* user_data, btck_Block* block, const btck_BlockValidationState* state);

/**
 * Function signature for serializing data.
 */
typedef int (*btck_WriteBytes)(const void* bytes, size_t size, void* userdata);

/**
 * Whether a validated data structure is valid, invalid, or an error was
 * encountered during processing.
 */
typedef uint8_t btck_ValidationMode;
#define btck_ValidationMode_VALID ((btck_ValidationMode)(0))
#define btck_ValidationMode_INVALID ((btck_ValidationMode)(1))
#define btck_ValidationMode_INTERNAL_ERROR ((btck_ValidationMode)(2))

/**
 * A granular "reason" why a block was invalid.
 */
typedef uint32_t btck_BlockValidationResult;
#define btck_BlockValidationResult_UNSET ((btck_BlockValidationResult)(0))           //!< initial value. Block has not yet been rejected
#define btck_BlockValidationResult_CONSENSUS ((btck_BlockValidationResult)(1))       //!< invalid by consensus rules (excluding any below reasons)
#define btck_BlockValidationResult_CACHED_INVALID ((btck_BlockValidationResult)(2))  //!< this block was cached as being invalid and we didn't store the reason why
#define btck_BlockValidationResult_INVALID_HEADER ((btck_BlockValidationResult)(3))  //!< invalid proof of work or time too old
#define btck_BlockValidationResult_MUTATED ((btck_BlockValidationResult)(4))         //!< the block's data didn't match the data committed to by the PoW
#define btck_BlockValidationResult_MISSING_PREV ((btck_BlockValidationResult)(5))    //!< We don't have the previous block the checked one is built on
#define btck_BlockValidationResult_INVALID_PREV ((btck_BlockValidationResult)(6))    //!< A block this one builds on is invalid
#define btck_BlockValidationResult_TIME_FUTURE ((btck_BlockValidationResult)(7))     //!< block timestamp was > 2 hours in the future (or our clock is bad)
#define btck_BlockValidationResult_HEADER_LOW_WORK ((btck_BlockValidationResult)(8)) //!< the block header may be on a too-little-work chain

/**
 * Holds the validation interface callbacks. The user data pointer may be used
 * to point to user-defined structures to make processing the validation
 * callbacks easier. Note that these callbacks block any further validation
 * execution when they are called.
 */
typedef struct {
    void* user_data;                                    //!< Holds a user-defined opaque structure that is passed to the validation
                                                        //!< interface callbacks. If user_data_destroy is also defined ownership of the
                                                        //!< user_data is passed to the created context options and subsequently context.
    btck_DestroyCallback user_data_destroy;             //!< Frees the provided user data structure.
    btck_ValidationInterfaceBlockChecked block_checked; //!< Called when a new block has been checked. Contains the
                                                        //!< result of its validation.
} btck_ValidationInterfaceCallbacks;

/**
 * A struct for holding the kernel notification callbacks. The user data
 * pointer may be used to point to user-defined structures to make processing
 * the notifications easier. Note that this makes it the user's responsibility
 * to ensure that the user_data outlives the kernel objects. Notifications can
 * occur even as kernel objects are deleted, so care has to be taken to ensure
 * safe unwinding.
 */
typedef struct {
    void* user_data;                        //!< Holds a user-defined opaque structure that is passed to the notification callbacks.
                                            //!< If user_data_destroy is also defined ownership of the user_data is passed to the
                                            //!< created context options and subsequently context.
    btck_DestroyCallback user_data_destroy; //!< Frees the provided user data structure.
    btck_NotifyBlockTip block_tip;          //!< The chain's tip was updated to the provided block entry.
    btck_NotifyHeaderTip header_tip;        //!< A new best block header was added.
    btck_NotifyProgress progress;           //!< Reports on current block synchronization progress.
    btck_NotifyWarningSet warning_set;      //!< A warning issued by the kernel library during validation.
    btck_NotifyWarningUnset warning_unset;  //!< A previous condition leading to the issuance of a warning is no longer given.
    btck_NotifyFlushError flush_error;      //!< An error encountered when flushing data to disk.
    btck_NotifyFatalError fatal_error;      //!< A un-recoverable system error encountered by the library.
} btck_NotificationInterfaceCallbacks;

/**
 * A collection of logging categories that may be encountered by kernel code.
 */
typedef uint8_t btck_LogCategory;
#define btck_LogCategory_ALL ((btck_LogCategory)(0))
#define btck_LogCategory_BENCH ((btck_LogCategory)(1))
#define btck_LogCategory_BLOCKSTORAGE ((btck_LogCategory)(2))
#define btck_LogCategory_COINDB ((btck_LogCategory)(3))
#define btck_LogCategory_LEVELDB ((btck_LogCategory)(4))
#define btck_LogCategory_MEMPOOL ((btck_LogCategory)(5))
#define btck_LogCategory_PRUNE ((btck_LogCategory)(6))
#define btck_LogCategory_RAND ((btck_LogCategory)(7))
#define btck_LogCategory_REINDEX ((btck_LogCategory)(8))
#define btck_LogCategory_VALIDATION ((btck_LogCategory)(9))
#define btck_LogCategory_KERNEL ((btck_LogCategory)(10))

/**
 * The level at which logs should be produced.
 */
typedef uint8_t btck_LogLevel;
#define btck_LogLevel_TRACE ((btck_LogLevel)(0))
#define btck_LogLevel_DEBUG ((btck_LogLevel)(1))
#define btck_LogLevel_INFO ((btck_LogLevel)(2))

/**
 * Options controlling the format of log messages.
 *
 * Set fields as non-zero to indicate true.
 */
typedef struct {
    int log_timestamps;               //!< Prepend a timestamp to log messages.
    int log_time_micros;              //!< Log timestamps in microsecond precision.
    int log_threadnames;              //!< Prepend the name of the thread to log messages.
    int log_sourcelocations;          //!< Prepend the source location to log messages.
    int always_print_category_levels; //!< Prepend the log category and level to log messages.
} btck_LoggingOptions;

/**
 * A collection of status codes that may be issued by the script verify function.
 */
typedef uint8_t btck_ScriptVerifyStatus;
#define btck_ScriptVerifyStatus_SCRIPT_VERIFY_OK ((btck_ScriptVerifyStatus)(0))
#define btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION ((btck_ScriptVerifyStatus)(2)) //!< The flags very combined in an invalid way.
#define btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED ((btck_ScriptVerifyStatus)(3))    //!< The taproot flag was set, so valid spent_outputs have to be provided.

/**
 * Script verification flags that may be composed with each other.
 */
typedef uint32_t btck_ScriptVerificationFlags;
#define btck_ScriptVerificationFlags_NONE ((btck_ScriptVerificationFlags)(0))
#define btck_ScriptVerificationFlags_P2SH ((btck_ScriptVerificationFlags)(1U << 0)) //!< evaluate P2SH (BIP16) subscripts
#define btck_ScriptVerificationFlags_DERSIG ((btck_ScriptVerificationFlags)(1U << 2)) //!< enforce strict DER (BIP66) compliance
#define btck_ScriptVerificationFlags_NULLDUMMY ((btck_ScriptVerificationFlags)(1U << 4)) //!< enforce NULLDUMMY (BIP147)
#define btck_ScriptVerificationFlags_CHECKLOCKTIMEVERIFY ((btck_ScriptVerificationFlags)(1U << 9)) //!< enable CHECKLOCKTIMEVERIFY (BIP65)
#define btck_ScriptVerificationFlags_CHECKSEQUENCEVERIFY ((btck_ScriptVerificationFlags)(1U << 10)) //!< enable CHECKSEQUENCEVERIFY (BIP112)
#define btck_ScriptVerificationFlags_WITNESS ((btck_ScriptVerificationFlags)(1U << 11)) //!< enable WITNESS (BIP141)
#define btck_ScriptVerificationFlags_TAPROOT ((btck_ScriptVerificationFlags)(1U << 17)) //!< enable TAPROOT (BIPs 341 & 342)
#define btck_ScriptVerificationFlags_ALL ((btck_ScriptVerificationFlags)(                              \
                                                    btck_ScriptVerificationFlags_P2SH |                \
                                                    btck_ScriptVerificationFlags_DERSIG |              \
                                                    btck_ScriptVerificationFlags_NULLDUMMY |           \
                                                    btck_ScriptVerificationFlags_CHECKLOCKTIMEVERIFY | \
                                                    btck_ScriptVerificationFlags_CHECKSEQUENCEVERIFY | \
                                                    btck_ScriptVerificationFlags_WITNESS |             \
                                                    btck_ScriptVerificationFlags_TAPROOT))

typedef uint8_t btck_ChainType;
#define btck_ChainType_MAINNET ((btck_ChainType)(0))
#define btck_ChainType_TESTNET ((btck_ChainType)(1))
#define btck_ChainType_TESTNET_4 ((btck_ChainType)(2))
#define btck_ChainType_SIGNET ((btck_ChainType)(3))
#define btck_ChainType_REGTEST ((btck_ChainType)(4))

/**
 * A type-safe block identifier.
 */
typedef struct {
    unsigned char hash[32];
} btck_BlockHash;

/** @name Transaction
 * Functions for working with transactions.
 */
///@{

/**
 * @brief Create a new transaction from the serialized data.
 *
 * @param[in] raw_transaction     Non-null.
 * @param[in] raw_transaction_len Length of the serialized transaction.
 * @return                        The transaction, or null on error.
 */
BITCOINKERNEL_API btck_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_create(
    const void* raw_transaction, size_t raw_transaction_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copy a transaction. Transactions are reference counted, so this just
 * increments the reference count.
 *
 * @param[in] transaction Non-null.
 * @return                The copied transaction.
 */
BITCOINKERNEL_API btck_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_copy(
    const btck_Transaction* transaction
) BITCOINKERNEL_ARG_NONNULL(1);

/*
 * @brief Serializes the transaction through the passed in callback to bytes.
 * This is consensus serialization that is also used for the p2p network.
 *
 * @param[in] transaction Non-null.
 * @param[in] writer      Non-null, callback to a write bytes function.
 * @param[in] user_data   Holds a user-defined opaque structure that will be
 *                        passed back through the writer callback.
 * @return                0 on success.
 */
BITCOINKERNEL_API int btck_transaction_to_bytes(
    const btck_Transaction* transaction,
    btck_WriteBytes writer,
    void* user_data
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Get the number of outputs of a transaction.
 *
 * @param[in] transaction Non-null.
 * @return                The number of outputs.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_count_outputs(
    const btck_Transaction* transaction
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the transaction outputs at the provided index. The returned
 * transaction output is not owned and depends on the lifetime of the
 * transaction.
 *
 * @param[in] transaction  Non-null.
 * @param[in] output_index The index of the transaction to be retrieved.
 * @return                 The transaction output
 */
BITCOINKERNEL_API btck_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_get_output_at(
    const btck_Transaction* transaction, size_t output_index
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the number of inputs of a transaction.
 *
 * @param[in] transaction Non-null.
 * @return                The number of inputs.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_count_inputs(
    const btck_Transaction* transaction
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction.
 */
BITCOINKERNEL_API void btck_transaction_destroy(btck_Transaction* transaction);

///@}

/** @name ScriptPubkey
 * Functions for working with script pubkeys.
 */
///@{

/**
 * @brief Create a script pubkey from serialized data.
 * @param[in] script_pubkey     Non-null.
 * @param[in] script_pubkey_len Length of the script pubkey data.
 * @return                      The script pubkey.
 */
BITCOINKERNEL_API btck_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT btck_script_pubkey_create(
    const void* script_pubkey, size_t script_pubkey_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copy a script pubkey.
 *
 * @param[in] script_pubkey Non-null.
 * @return                  The copied script pubkey.
 */
BITCOINKERNEL_API btck_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT btck_script_pubkey_copy(
    const btck_ScriptPubkey* script_pubkey
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Verify if the input at input_index of tx_to spends the script pubkey
 * under the constraints specified by flags. If the
 * `btck_SCRIPT_FLAGS_VERIFY_WITNESS` flag is set in the flags bitfield, the
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
 * @param[in] flags             Bitfield of btck_ScriptFlags controlling validation constraints.
 * @param[out] status           Nullable, will be set to an error code if the operation fails.
 *                              Should be set to btck_SCRIPT_VERIFY_OK.
 * @return                      1 if the script is valid, 0 otherwise.
 */
BITCOINKERNEL_API int BITCOINKERNEL_WARN_UNUSED_RESULT btck_script_pubkey_verify(
    const btck_ScriptPubkey* script_pubkey,
    int64_t amount,
    const btck_Transaction* tx_to,
    const btck_TransactionOutput** spent_outputs, size_t spent_outputs_len,
    unsigned int input_index,
    unsigned int flags,
    btck_ScriptVerifyStatus* status
) BITCOINKERNEL_ARG_NONNULL(1, 3);

/*
 * @brief Serializes the script pubkey through the passed in callback to bytes.
 *
 * @param[in] script_pubkey Non-null.
 * @param[in] writer        Non-null, callback to a write bytes function.
 * @param[in] user_data     Holds a user-defined opaque structure that will be
 *                          passed back through the writer callback.
 * @return                  0 on success.
 */
BITCOINKERNEL_API int btck_script_pubkey_to_bytes(
    const btck_ScriptPubkey* script_pubkey,
    btck_WriteBytes writer,
    void* user_data
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * Destroy the script pubkey.
 */
BITCOINKERNEL_API void btck_script_pubkey_destroy(btck_ScriptPubkey* script_pubkey);

///@}

/** @name TransactionOutput
 * Functions for working with transaction outputs.
 */
///@{

/**
 * @brief Create a transaction output from a script pubkey and an amount.
 *
 * @param[in] script_pubkey Non-null.
 * @param[in] amount        The amount associated with the script pubkey for this output.
 * @return                  The transaction output.
 */
BITCOINKERNEL_API btck_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_output_create(
    const btck_ScriptPubkey* script_pubkey,
    int64_t amount
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the script pubkey of the output. The returned
 * script pubkey is not owned and depends on the lifetime of the
 * transaction output.
 *
 * @param[in] transaction_output Non-null.
 * @return                       The script pubkey.
 */
BITCOINKERNEL_API btck_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_output_get_script_pubkey(
        const btck_TransactionOutput* transaction_output
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the amount in the output.
 *
 * @param[in] transaction_output Non-null.
 * @return                       The amount.
 */
BITCOINKERNEL_API int64_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_output_get_amount(
    const btck_TransactionOutput* transaction_output
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 *  @brief Copy a transaction output.
 *
 *  @param[in] transaction_output Non-null.
 *  @return                       The copied transaction output.
 */
BITCOINKERNEL_API btck_TransactionOutput* btck_transaction_output_copy(
    const btck_TransactionOutput* transaction_output
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction output.
 */
BITCOINKERNEL_API void btck_transaction_output_destroy(btck_TransactionOutput* transaction_output);

///@}

/** @name Logging
 * Logging-related functions.
 */
///@{

/**
 * @brief This disables the global internal logger. No log messages will be
 * buffered internally anymore once this is called and the buffer is cleared.
 * This function should only be called once and is not thread or re-entry safe.
 * Log messages will be buffered until this function is called, or a logging
 * connection is created.
 */
BITCOINKERNEL_API void btck_logging_disable();

/**
 * @brief Set the log level of the global internal logger. This does not
 * enable the selected categories. Use @ref btck_logging_enable_category to
 * start logging from a specific, or all categories. This function is not
 * thread safe. Multiple calls from different threads are allowed but must be
 * synchronized. This changes a global setting and will override settings for
 * all existing @ref btck_LoggingConnection instances.
 *
 * @param[in] category If btck_LOG_ALL is chosen, all messages at the specified level
 *                     will be logged. Otherwise only messages from the specified category
 *                     will be logged at the specified level and above.
 * @param[in] level    Log level at which the log category is set.
 */
BITCOINKERNEL_API void btck_logging_set_level_category(btck_LogCategory category, btck_LogLevel level);

/**
 * @brief Enable a specific log category for the global internal logger. This
 * function is not thread safe. Multiple calls from different threads are
 * allowed but must be synchronized. This changes a global setting and will
 * override settings for all existing @ref btck_LoggingConnection instances.
 *
 * @param[in] category If btck_LOG_ALL is chosen, all categories will be enabled.
 */
BITCOINKERNEL_API void btck_logging_enable_category(btck_LogCategory category);

/**
 * @brief Disable a specific log category for the global internal logger. This
 * function is not thread safe. Multiple calls from different threads are
 * allowed but must be synchronized. This changes a global setting and will
 * override settings for all existing @ref btck_LoggingConnection instances.
 *
 * @param[in] category If btck_LOG_ALL is chosen, all categories will be disabled.
 */
BITCOINKERNEL_API void btck_logging_disable_category(btck_LogCategory category);

/**
 * @brief Start logging messages through the provided callback. Log messages
 * produced before this function is first called are buffered and on calling this
 * function are logged immediately.
 *
 * @param[in] log_callback               Non-null, function through which messages will be logged.
 * @param[in] user_data                  Nullable, holds a user-defined opaque structure. Is passed back
 *                                       to the user through the callback. If the user_data_destroy_callback
 *                                       is also defined it is assumed that ownership of the user_data is passed
 *                                       to the created logging connection.
 * @param[in] user_data_destroy_callback Nullable, function for freeing the user data.
 * @param[in] options                    Sets formatting options of the log messages.
 * @return                               A new kernel logging connection, or null on error.
 */
BITCOINKERNEL_API btck_LoggingConnection* BITCOINKERNEL_WARN_UNUSED_RESULT btck_logging_connection_create(
    btck_LogCallback log_callback,
    void* user_data,
    btck_DestroyCallback user_data_destroy_callback,
    const btck_LoggingOptions options
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Stop logging and destroy the logging connection.
 */
BITCOINKERNEL_API void btck_logging_connection_destroy(btck_LoggingConnection* logging_connection);

///@}

/** @name ChainParameters
 * Functions for working with chain parameters.
 */
///@{

/**
 * @brief Creates a chain parameters struct with default parameters based on the
 * passed in chain type.
 *
 * @param[in] chain_type Controls the chain parameters type created.
 * @return               An allocated chain parameters opaque struct.
 */
BITCOINKERNEL_API btck_ChainParameters* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chain_parameters_create(
    const btck_ChainType chain_type);

/**
 * Destroy the chain parameters.
 */
BITCOINKERNEL_API void btck_chain_parameters_destroy(btck_ChainParameters* chain_parameters);

///@}

/** @name ContextOptions
 * Functions for working with context options.
 */
///@{

/**
 * Creates an empty context options.
 */
BITCOINKERNEL_API btck_ContextOptions* BITCOINKERNEL_WARN_UNUSED_RESULT btck_context_options_create();

/**
 * @brief Sets the chain params for the context options. The context created
 * with the options will be configured for these chain parameters.
 *
 * @param[in] context_options  Non-null, previously created by @ref btck_context_options_create.
 * @param[in] chain_parameters Is set to the context options.
 */
BITCOINKERNEL_API void btck_context_options_set_chainparams(
    btck_ContextOptions* context_options,
    const btck_ChainParameters* chain_parameters
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Set the kernel notifications for the context options. The context
 * created with the options will be configured with these notifications.
 *
 * @param[in] context_options Non-null, previously created by @ref btck_context_options_create.
 * @param[in] notifications   Is set to the context options.
 */
BITCOINKERNEL_API void btck_context_options_set_notifications(
    btck_ContextOptions* context_options,
    btck_NotificationInterfaceCallbacks notifications
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Set the validation interface callbacks for the context options. The
 * context created with the options will be configured for these validation
 * interface callbacks. The callbacks will then be triggered from validation
 * events issued by the chainstate manager created from the same context.
 *
 * @param[in] context_options                Non-null, previously created with btck_context_options_create.
 * @param[in] validation_interface_callbacks The callbacks used for passing validation information to the
 *                                           user.
 */
BITCOINKERNEL_API void btck_context_options_set_validation_interface(
    btck_ContextOptions* context_options,
    btck_ValidationInterfaceCallbacks validation_interface_callbacks
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the context options.
 */
BITCOINKERNEL_API void btck_context_options_destroy(btck_ContextOptions* context_options);

///@}

/** @name Context
 * Functions for working with contexts.
 */
///@{

/**
 * @brief Create a new kernel context. If the options have not been previously
 * set, their corresponding fields will be initialized to default values; the
 * context will assume mainnet chain parameters and won't attempt to call the
 * kernel notification callbacks.
 *
 * @param[in] context_options Nullable, created by @ref btck_context_options_create.
 * @return                    The allocated context, or null on error.
 */
BITCOINKERNEL_API btck_Context* BITCOINKERNEL_WARN_UNUSED_RESULT btck_context_create(
    const btck_ContextOptions* context_options);

/**
 * @brief Interrupt can be used to halt long-running validation functions like
 * when reindexing, importing or processing blocks.
 *
 * @param[in] context  Non-null.
 * @return             0 if the interrupt was successfully, non-zero otherwise.
 */
BITCOINKERNEL_API int BITCOINKERNEL_WARN_UNUSED_RESULT btck_context_interrupt(
    btck_Context* context
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the context.
 */
BITCOINKERNEL_API void btck_context_destroy(btck_Context* context);

///@}

/** @name BlockTreeEntry
 * Functions for working with block tree entries.
 */
///@{

/**
 * @brief Returns the previous block tree entry in the chain, or null if the current
 * block tree entry is the genesis block.
 *
 * @param[in] block_tree_entry Non-null.
 * @return                     The previous block tree entry, or null on error or if the current block tree entry is the genesis block.
 */
BITCOINKERNEL_API btck_BlockTreeEntry* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_tree_entry_get_previous(
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Return the height of a certain block tree entry.
 *
 * @param[in] block_tree_entry Non-null.
 * @return                     The block height.
 */
BITCOINKERNEL_API int32_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_tree_entry_get_height(
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Destroy the block tree entry.
 */
BITCOINKERNEL_API void btck_block_tree_entry_destroy(btck_BlockTreeEntry* block_tree_entry);

///@}

/** @name ChainstateManagerOptions
 * Functions for working with chainstate manager options.
 */
///@{

/**
 * @brief Create options for the chainstate manager.
 *
 * @param[in] context          Non-null, the created options and through it the chainstate manager will
                               associate with this kernel context for the duration of their lifetimes.
 * @param[in] data_directory   Non-null, path string of the directory containing the chainstate data.
 *                             If the directory does not exist yet, it will be created.
 * @param[in] blocks_directory Non-null, path string of the directory containing the block data. If
 *                             the directory does not exist yet, it will be created.
 * @return                     The allocated chainstate manager options, or null on error.
 */
BITCOINKERNEL_API btck_ChainstateManagerOptions* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chainstate_manager_options_create(
    const btck_Context* context,
    const char* data_directory,
    size_t data_directory_len,
    const char* blocks_directory,
    size_t blocks_directory_len
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Set the number of available worker threads used during validation.
 *
 * @param[in] chainstate_manager_options Non-null, options to be set.
 * @param[in] worker_threads             The number of worker threads that should be spawned in the thread pool
 *                                       used for validation. When set to 0 no parallel verification is done.
 *                                       The value range is clamped internally between 0 and 15.
 */
BITCOINKERNEL_API void btck_chainstate_manager_options_set_worker_threads_num(
        btck_ChainstateManagerOptions* chainstate_manager_options,
        int worker_threads
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets wipe db in the options. In combination with calling
 * @ref btck_chainstate_manager_import_blocks this triggers either a full reindex,
 * or a reindex of just the chainstate database.
 *
 * @param[in] chainstate_manager_options Non-null, created by @ref btck_chainstate_manager_options_create.
 * @param[in] wipe_block_tree_db         Set wipe block tree db. Should only be 1 if wipe_chainstate_db is 1 too.
 * @param[in] wipe_chainstate_db         Set wipe chainstate db.
 * @return                               0 if the set was successful, non-zero if the set failed.
 */
BITCOINKERNEL_API int btck_chainstate_manager_options_set_wipe_dbs(
    btck_ChainstateManagerOptions* chainstate_manager_options,
    int wipe_block_tree_db,
    int wipe_chainstate_db
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets block tree db in memory in the options.
 *
 * @param[in] chainstate_manager_options   Non-null, created by @ref btck_chainstate_manager_options_create.
 * @param[in] block_tree_db_in_memory      Set block tree db in memory.
 */
BITCOINKERNEL_API void btck_chainstate_manager_options_set_block_tree_db_in_memory(
    btck_ChainstateManagerOptions* chainstate_manager_options,
    int block_tree_db_in_memory
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Sets chainstate db in memory in the options.
 *
 * @param[in] chainstate_manager_options Non-null, created by @ref btck_chainstate_manager_options_create.
 * @param[in] chainstate_db_in_memory    Set chainstate db in memory.
 */
BITCOINKERNEL_API void btck_chainstate_manager_options_set_chainstate_db_in_memory(
    btck_ChainstateManagerOptions* chainstate_manager_options,
    int chainstate_db_in_memory
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the chainstate manager options.
 */
BITCOINKERNEL_API void btck_chainstate_manager_options_destroy(btck_ChainstateManagerOptions* chainstate_manager_options);

///@}

/** @name ChainstateManager
 * Functions for chainstate management.
 */
///@{

/**
 * @brief Create a chainstate manager. This is the main object for many
 * validation tasks as well as for retrieving data from the chain and
 * interacting with its chainstate and indexes.
 *
 * @param[in] chainstate_manager_options Non-null, created by @ref btck_chainstate_manager_options_create.
 * @return                               The allocated chainstate manager, or null on error.
 */
BITCOINKERNEL_API btck_ChainstateManager* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chainstate_manager_create(
    const btck_ChainstateManagerOptions* chainstate_manager_options
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief May be called once the btck_ChainstateManager is instantiated.
 * Triggers the start of a reindex if the option was previously set for the
 * chainstate and block manager. Can also import an array of existing block
 * files selected by the user.
 *
 * @param[in] chainstate_manager   Non-null.
 * @param[in] block_file_paths     Nullable, array of block files described by their full filesystem paths.
 * @param[in] block_file_paths_len Length of the block_file_paths array.
 * @return                         0 if the import blocks call was completed successfully, non-zero otherwise.
 */
BITCOINKERNEL_API int btck_chainstate_manager_import_blocks(
    btck_ChainstateManager* chainstate_manager,
    const char** block_file_paths, size_t* block_file_paths_lens,
    size_t block_file_paths_len
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Process and validate the passed in block with the chainstate
 * manager. More detailed validation information in case of a failure can also
 * be retrieved through a registered validation interface. If the block fails
 * to validate the `block_checked` callback's 'BlockValidationState' will
 * contain details.
 *
 * @param[in] chainstate_manager Non-null.
 * @param[in] block              Non-null, block to be validated.
 * @param[out] new_block         Nullable, will be set to 1 if this block was not processed before.
 * @return                       0 if processing the block was successful. Will also return 0 for valid, but duplicate blocks.
 */
BITCOINKERNEL_API int BITCOINKERNEL_WARN_UNUSED_RESULT btck_chainstate_manager_process_block(
    btck_ChainstateManager* chainstate_manager,
    const btck_Block* block,
    int* new_block
) BITCOINKERNEL_ARG_NONNULL(1, 2, 3);

/**
 * @brief Returns the best known currently active chain. Its lifetime is
 * dependent on the chainstate manager and state transitions within the
 * chainstate manager, e.g. when processing blocks, will also change the chain.
 * Data retrieved from this chain is only consistent up to the point when new
 * data is processed in the chainstate manager. It is the user's responsibility
 * to guard against these inconsistencies.
 *
 * @param[in] chainstate_manager Non-null.
 * @return                       The chain.
 */
BITCOINKERNEL_API btck_Chain* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chainstate_manager_get_active_chain(
    const btck_ChainstateManager* chainstate_manager
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Retrieve a block tree entry by its block hash.
 *
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_hash         Non-null.
 * @return                       The block tree entry of the block with the passed in hash, or null if
 *                               the block hash is not found.
 */
BITCOINKERNEL_API btck_BlockTreeEntry* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chainstate_manager_get_block_tree_entry_by_hash(
    const btck_ChainstateManager* chainstate_manager,
    const btck_BlockHash* block_hash
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * Destroy the chainstate manager.
 */
BITCOINKERNEL_API void btck_chainstate_manager_destroy(btck_ChainstateManager* chainstate_manager);

///@}

/** @name Block
 * Functions for working with blocks.
 */
///@{

/**
 * @brief Reads the block the passed in block index points to from disk and
 * returns it.
 *
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_tree_entry   Non-null.
 * @return                       The read out block, or null on error.
 */
BITCOINKERNEL_API btck_Block* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_read(
    const btck_ChainstateManager* chainstate_manager,
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Parse a serialized raw block into a new block object.
 *
 * @param[in] raw_block     Non-null, serialized block.
 * @param[in] raw_block_len Length of the serialized block.
 * @return                  The allocated block, or null on error.
 */
BITCOINKERNEL_API btck_Block* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_create(
    const void* raw_block, size_t raw_block_len
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copy a block. Blocks are reference counted, so this just increments
 * the reference count.
 *
 * @param[in] block Non-null.
 * @return          The copied block.
 */
BITCOINKERNEL_API btck_Block* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_copy(
    const btck_Block* block
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Count the number of transactions contained in a block.
 *
 * @param[in] block Non-null.
 * @return          The number of transactions in the block.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_count_transactions(
    const btck_Block* block
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the transaction at the provided index. The returned transaction
 * is owned and does not depend on the lifetime of the block.
 *
 * @param[in] block             Non-null.
 * @param[in] transaction_index The index of the transaction to be retrieved.
 * @return                      The transaction.
 */
BITCOINKERNEL_API btck_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_get_transaction_at(
    const btck_Block* block, size_t transaction_index
) BITCOINKERNEL_ARG_NONNULL(1);

/*
 * @brief Calculate and return the hash of a block.
 *
 * @param[in] block Non-null.
 * @return    The block hash.
 */
BITCOINKERNEL_API btck_BlockHash* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_get_hash(
    const btck_Block* block
) BITCOINKERNEL_ARG_NONNULL(1);

/*
 * @brief Serializes the block through the passed in callback to bytes.
 * This is consensus serialization that is also used for the p2p network.
 *
 * @param[in] block     Non-null.
 * @param[in] writer    Non-null, callback to a write bytes function.
 * @param[in] user_data Holds a user-defined opaque structure that will be
 *                      passed back through the writer callback.
 * @return              True on success.
 */
BITCOINKERNEL_API int btck_block_to_bytes(
    const btck_Block* block,
    btck_WriteBytes writer,
    void* user_data
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * Destroy the block.
 */
BITCOINKERNEL_API void btck_block_destroy(btck_Block* block);

///@}

/** @name BlockValidationState
 * Functions for working with block validation states.
 */
///@{

/**
 * Returns the validation mode from an opaque block validation state pointer.
 */
BITCOINKERNEL_API btck_ValidationMode btck_block_validation_state_get_validation_mode(
    const btck_BlockValidationState* block_validation_state
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Returns the validation result from an opaque block validation state pointer.
 */
BITCOINKERNEL_API btck_BlockValidationResult btck_block_validation_state_get_block_validation_result(
    const btck_BlockValidationState* block_validation_state
) BITCOINKERNEL_ARG_NONNULL(1);

///@}

/** @name Chain
 * Functions for working with the chain
 */
///@{

/**
 * @brief Get the block tree entry of the current chain tip. Once returned,
 * there is no guarantee that it remains in the active chain.
 *
 * @param[in] chain Non-null.
 * @return          The block tree entry of the current tip, or null if the chain is empty.
 */
BITCOINKERNEL_API btck_BlockTreeEntry* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chain_get_tip(
    const btck_Chain* chain
) BITCOINKERNEL_ARG_NONNULL(1);

/*
 * @brief Get the block tree entry of the genesis block.
 *
 * @param[in] chain Non-null.
 * @return          The block tree entry of the genesis block, or null if the chain is empty.
 */
BITCOINKERNEL_API btck_BlockTreeEntry* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chain_get_genesis(
    const btck_Chain* chain
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Retrieve a block tree entry by its height in the currently active chain.
 * Once retrieved there is no guarantee that it remains in the active chain.
 *
 * @param[in] chain        Non-null.
 * @param[in] block_height Height in the chain of the to be retrieved block tree entry.
 * @return                 The block tree entry at a certain height in the currently active chain.
 */
BITCOINKERNEL_API btck_BlockTreeEntry* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chain_get_by_height(
    const btck_Chain* chain,
    int block_height
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Return true if the passed in chain contains the block tree entry.
 *
 * @param[in] chain            Non-null.
 * @param[in] block_tree_entry Non-null.
 * @return                     1 if the block_tree_entry is in the chain, 0 otherwise.
 *
 */
BITCOINKERNEL_API int btck_chain_contains(
    const btck_Chain* chain,
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Destroy the chain.
 */
BITCOINKERNEL_API void btck_chain_destroy(btck_Chain* chain);

///@}

/** @name BlockSpentOutputs
 * Functions for working with block spent outputs.
 */
///@{

/**
 * @brief Reads the block spent coins data the passed in block tree entry points to from
 * disk and returns it.
 *
 * @param[in] chainstate_manager Non-null.
 * @param[in] block_tree_entry   Non-null.
 * @return                       The read out block spent outputs, or null on error.
 */
BITCOINKERNEL_API btck_BlockSpentOutputs* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_spent_outputs_read(
    const btck_ChainstateManager* chainstate_manager,
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Copy a block's spent outputs.
 *
 * @param[in] block_spent_outputs Non-null.
 * @return                        The copied block spent outputs.
 */
BITCOINKERNEL_API btck_BlockSpentOutputs* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_spent_outputs_copy(
    const btck_BlockSpentOutputs* block_spent_outputs
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns the number of transaction spent outputs whose data is contained in
 * block spent outputs.
 *
 * @param[in] block_spent_outputs Non-null.
 * @return                        The number of transaction spent outputs data in the block spent outputs.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_spent_outputs_count(
    const btck_BlockSpentOutputs* block_spent_outputs
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns a transaction spent outputs contained in the block spent
 * outputs at a certain index. The returned pointer is unowned and only valid
 * for the lifetime of block_spent_outputs.
 *
 * @param[in] block_spent_outputs             Non-null.
 * @param[in] transaction_spent_outputs_index The index of the transaction spent outputs within the block spent outputs.
 * @return                                    A transaction spent outputs pointer.
 */
BITCOINKERNEL_API btck_TransactionSpentOutputs* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_spent_outputs_get_transaction_spent_outputs_at(
    const btck_BlockSpentOutputs* block_spent_outputs,
    size_t transaction_spent_outputs_index) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the block spent outputs.
 */
BITCOINKERNEL_API void btck_block_spent_outputs_destroy(btck_BlockSpentOutputs* block_spent_outputs);

///@}

/** @name TransactionSpentOutputs
 * Functions for working with the spent coins of a transaction
 */
///@{

/**
 * @brief Copy a transaction's spent outputs.
 *
 * @param[in] transaction_spent_outputs Non-null.
 * @return                              The copied transaction spent outputs.
 */
BITCOINKERNEL_API btck_TransactionSpentOutputs* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_spent_outputs_copy(
    const btck_TransactionSpentOutputs* transaction_spent_outputs
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns the number of previous transaction outputs contained in the
 * transaction spent outputs data.
 *
 * @param[in] transaction_spent_outputs Non-null
 * @return                              The number of spent transaction outputs for the transaction.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_spent_outputs_count(
    const btck_TransactionSpentOutputs* transaction_spent_outputs
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns a coin contained in the transaction spent outputs at a
 * certain index. The returned pointer is unowned and only valid for the
 * lifetime of transaction_spent_outputs.
 *
 * @param[in] transaction_spent_outputs Non-null.
 * @param[in] coin_index                The index of the to be retrieved coin within the
 *                                      transaction spent outputs.
 * @return                              A coin pointer.
 */
BITCOINKERNEL_API btck_Coin* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_spent_outputs_get_coin_at(
    const btck_TransactionSpentOutputs* transaction_spent_outputs,
    size_t coin_index) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the transaction spent outputs.
 */
BITCOINKERNEL_API void btck_transaction_spent_outputs_destroy(btck_TransactionSpentOutputs* transaction_spent_outputs);

///@}

/** @name Coin
 * Functions for working with coins.
 */
///@{

/**
 * @brief Copy a coin.
 *
 * @param[in] coin Non-null.
 * @return         The copied coin.
 */
BITCOINKERNEL_API btck_Coin* BITCOINKERNEL_WARN_UNUSED_RESULT btck_coin_copy(
    const btck_Coin* coin
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns the height of the block that contains the coin's prevout.
 *
 * @param[in] coin Non-null.
 * @return         The block height of the coin.
 */
BITCOINKERNEL_API uint32_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_coin_confirmation_height(
    const btck_Coin* coin
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Returns whether the containing transaction was a coinbase.
 *
 * @param[in] coin Non-null.
 * @return         1 if the coin is a coinbase coin, 0 otherwise.
 */
BITCOINKERNEL_API int BITCOINKERNEL_WARN_UNUSED_RESULT btck_coin_is_coinbase(
    const btck_Coin* coin
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Return the transaction output of a coin. The returned pointer is
 * unowned and only valid for the lifetime of the coin.
 *
 * @param[in] coin Non-null.
 * @return         A transaction output pointer.
 */
BITCOINKERNEL_API btck_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT btck_coin_get_output(
    const btck_Coin* coin
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the coin.
 */
BITCOINKERNEL_API void btck_coin_destroy(btck_Coin* coin);

///@}

/** @name BlockHash
 * Functions for working with block hashes.
 */
///@{

/**
 * @brief Return the block hash associated with a block tree entry.
 *
 * @param[in] block_tree_entry Non-null.
 * @return                     The block hash.
 */
BITCOINKERNEL_API btck_BlockHash* BITCOINKERNEL_WARN_UNUSED_RESULT btck_block_tree_entry_get_block_hash(
    const btck_BlockTreeEntry* block_tree_entry
) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the block hash.
 */
BITCOINKERNEL_API void btck_block_hash_destroy(btck_BlockHash* block_hash);

///@}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
