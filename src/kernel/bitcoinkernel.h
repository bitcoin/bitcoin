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
        #else
            #define BITCOINKERNEL_API __attribute__((visibility("default")))
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
 * Const pointers represent views, and do not transfer ownership. Lifetime
 * guarantees of these objects are described in the respective documentation.
 * Ownership of these resources may be taken by copying. They are typically
 * used for iteration with minimal overhead and require some care by the
 * programmer that their lifetime is not extended beyond that of the original
 * object.
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
 * global and change the settings for all existing btck_LoggingConnection
 * instances.
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
 * parameters and callbacks for handling error and validation events. Once
 * other validation objects are instantiated from it, the context is kept in
 * memory for the duration of their lifetimes.
 *
 * A constructed context can be safely used from multiple threads.
 */
typedef struct btck_Context btck_Context;

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
#define btck_ScriptVerifyStatus_ERROR_INVALID_FLAGS_COMBINATION ((btck_ScriptVerifyStatus)(2)) //!< The flags were combined in an invalid way.
#define btck_ScriptVerifyStatus_ERROR_SPENT_OUTPUTS_REQUIRED ((btck_ScriptVerifyStatus)(3))    //!< The taproot flag was set, so valid spent_outputs have to be provided.

/**
 * Script verification flags that may be composed with each other.
 */
typedef uint32_t btck_ScriptVerificationFlags;
#define btck_ScriptVerificationFlags_NONE ((btck_ScriptVerificationFlags)(0))
#define btck_ScriptVerificationFlags_P2SH ((btck_ScriptVerificationFlags)(1U << 0))                 //!< evaluate P2SH (BIP16) subscripts
#define btck_ScriptVerificationFlags_DERSIG ((btck_ScriptVerificationFlags)(1U << 2))               //!< enforce strict DER (BIP66) compliance
#define btck_ScriptVerificationFlags_NULLDUMMY ((btck_ScriptVerificationFlags)(1U << 4))            //!< enforce NULLDUMMY (BIP147)
#define btck_ScriptVerificationFlags_CHECKLOCKTIMEVERIFY ((btck_ScriptVerificationFlags)(1U << 9))  //!< enable CHECKLOCKTIMEVERIFY (BIP65)
#define btck_ScriptVerificationFlags_CHECKSEQUENCEVERIFY ((btck_ScriptVerificationFlags)(1U << 10)) //!< enable CHECKSEQUENCEVERIFY (BIP112)
#define btck_ScriptVerificationFlags_WITNESS ((btck_ScriptVerificationFlags)(1U << 11))             //!< enable WITNESS (BIP141)
#define btck_ScriptVerificationFlags_TAPROOT ((btck_ScriptVerificationFlags)(1U << 17))             //!< enable TAPROOT (BIPs 341 & 342)
#define btck_ScriptVerificationFlags_ALL ((btck_ScriptVerificationFlags)(btck_ScriptVerificationFlags_P2SH |                \
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
 * Function signature for serializing data.
 */
typedef int (*btck_WriteBytes)(const void* bytes, size_t size, void* userdata);

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
    const void* raw_transaction, size_t raw_transaction_len) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copy a transaction. Transactions are reference counted, so this just
 * increments the reference count.
 *
 * @param[in] transaction Non-null.
 * @return                The copied transaction.
 */
BITCOINKERNEL_API btck_Transaction* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_copy(
    const btck_Transaction* transaction) BITCOINKERNEL_ARG_NONNULL(1);

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
    void* user_data) BITCOINKERNEL_ARG_NONNULL(1, 2);

/**
 * @brief Get the number of outputs of a transaction.
 *
 * @param[in] transaction Non-null.
 * @return                The number of outputs.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_count_outputs(
    const btck_Transaction* transaction) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the transaction outputs at the provided index. The returned
 * transaction output is not owned and depends on the lifetime of the
 * transaction.
 *
 * @param[in] transaction  Non-null.
 * @param[in] output_index The index of the transaction to be retrieved.
 * @return                 The transaction output
 */
BITCOINKERNEL_API const btck_TransactionOutput* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_get_output_at(
    const btck_Transaction* transaction, size_t output_index) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the number of inputs of a transaction.
 *
 * @param[in] transaction Non-null.
 * @return                The number of inputs.
 */
BITCOINKERNEL_API size_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_count_inputs(
    const btck_Transaction* transaction) BITCOINKERNEL_ARG_NONNULL(1);

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
    const void* script_pubkey, size_t script_pubkey_len) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Copy a script pubkey.
 *
 * @param[in] script_pubkey Non-null.
 * @return                  The copied script pubkey.
 */
BITCOINKERNEL_API btck_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT btck_script_pubkey_copy(
    const btck_ScriptPubkey* script_pubkey) BITCOINKERNEL_ARG_NONNULL(1);

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
    btck_ScriptVerifyStatus* status) BITCOINKERNEL_ARG_NONNULL(1, 3);

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
    void* user_data) BITCOINKERNEL_ARG_NONNULL(1, 2);

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
    int64_t amount) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the script pubkey of the output. The returned
 * script pubkey is not owned and depends on the lifetime of the
 * transaction output.
 *
 * @param[in] transaction_output Non-null.
 * @return                       The script pubkey.
 */
BITCOINKERNEL_API const btck_ScriptPubkey* BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_output_get_script_pubkey(
    const btck_TransactionOutput* transaction_output) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * @brief Get the amount in the output.
 *
 * @param[in] transaction_output Non-null.
 * @return                       The amount.
 */
BITCOINKERNEL_API int64_t BITCOINKERNEL_WARN_UNUSED_RESULT btck_transaction_output_get_amount(
    const btck_TransactionOutput* transaction_output) BITCOINKERNEL_ARG_NONNULL(1);

/**
 *  @brief Copy a transaction output.
 *
 *  @param[in] transaction_output Non-null.
 *  @return                       The copied transaction output.
 */
BITCOINKERNEL_API btck_TransactionOutput* btck_transaction_output_copy(
    const btck_TransactionOutput* transaction_output) BITCOINKERNEL_ARG_NONNULL(1);

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
 * start logging from a specific, or all categories. This changes a global
 * setting and will override settings for all existing
 * @ref btck_LoggingConnection instances.
 *
 * @param[in] category If btck_LOG_ALL is chosen, all messages at the specified level
 *                     will be logged. Otherwise only messages from the specified category
 *                     will be logged at the specified level and above.
 * @param[in] level    Log level at which the log category is set.
 */
BITCOINKERNEL_API void btck_logging_set_level_category(btck_LogCategory category, btck_LogLevel level);

/**
 * @brief Enable a specific log category for the global internal logger. This
 * changes a global setting and will override settings for all existing @ref
 * btck_LoggingConnection instances.
 *
 * @param[in] category If btck_LOG_ALL is chosen, all categories will be enabled.
 */
BITCOINKERNEL_API void btck_logging_enable_category(btck_LogCategory category);

/**
 * @brief Disable a specific log category for the global internal logger. This
 * changes a global setting and will override settings for all existing @ref
 * btck_LoggingConnection instances.
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
    const btck_LoggingOptions options) BITCOINKERNEL_ARG_NONNULL(1);

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
 * Copy the chain parameters.
 */
BITCOINKERNEL_API btck_ChainParameters* BITCOINKERNEL_WARN_UNUSED_RESULT btck_chain_parameters_copy(
    const btck_ChainParameters* chain_parameters) BITCOINKERNEL_ARG_NONNULL(1);

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
    const btck_ChainParameters* chain_parameters) BITCOINKERNEL_ARG_NONNULL(1, 2);

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
 * @return                    The allocated kernel context, or null on error.
 */
BITCOINKERNEL_API btck_Context* BITCOINKERNEL_WARN_UNUSED_RESULT btck_context_create(
    const btck_ContextOptions* context_options);

/**
 * Copy the context.
 */
BITCOINKERNEL_API btck_Context* BITCOINKERNEL_WARN_UNUSED_RESULT btck_context_copy(
    const btck_Context* context) BITCOINKERNEL_ARG_NONNULL(1);

/**
 * Destroy the context.
 */
BITCOINKERNEL_API void btck_context_destroy(btck_Context* context);

///@}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BITCOIN_KERNEL_BITCOINKERNEL_H
