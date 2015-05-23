// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOINCONSENSUS_H
#define BITCOIN_BITCOINCONSENSUS_H

#include <stddef.h>

#if defined(BUILD_BITCOIN_INTERNAL) && defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
  #if defined(_WIN32)
    #if defined(DLL_EXPORT)
      #if defined(HAVE_FUNC_ATTRIBUTE_DLLEXPORT)
        #define EXPORT_SYMBOL __declspec(dllexport)
      #else
        #define EXPORT_SYMBOL
      #endif
    #endif
  #elif defined(HAVE_FUNC_ATTRIBUTE_VISIBILITY)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBBITCOINCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BITCOINCONSENSUS_API_VER 0

typedef enum bitcoinconsensus_error_t
{
    bitcoinconsensus_ERR_OK = 0,
    bitcoinconsensus_ERR_TX_INDEX,
    bitcoinconsensus_ERR_TX_SIZE_MISMATCH,
    bitcoinconsensus_ERR_TX_DESERIALIZE,
    bitcoinconsensus_ERR_UNKNOWN,
    bitcoinconsensus_ERR_BAD_INDEX,
    bitcoinconsensus_ERR_SCRIPT_EXECUTION,
} bitcoinconsensus_error;

/** Script verification flags */
enum
{
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_NONE      = 0,
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH      = (1U << 0), // evaluate P2SH (BIP16) subscripts
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG    = (1U << 2), // enforce strict DER (BIP66) compliance
};

typedef enum bitcoinconsensus_script_execution_stack
{
    bitcoinconsensus_SEXEC_STACK,
    bitcoinconsensus_SEXEC_ALTSTACK,
} bitcoinconsensus_script_execution_stack;

typedef struct bitcoinconsensus_script_execution_t_ bitcoinconsensus_script_execution_t;

/// Returns an opaque pointer representing a context for execution of the given script for a given transaction.
/// If txTo is NULL, all operations that access the transaction will fail.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL bitcoinconsensus_script_execution_t *bitcoinconsensus_script_execution(const unsigned char *script, unsigned int scriptLen, const unsigned char *txTo, unsigned int txToLen, unsigned int nIn, bitcoinconsensus_error*);

/// Inserts a blob of data at a given position on one of the stacks.
/// To access the altstack, the execution context must already be started.
/// A negative stackPos will insert at the end of the stack.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_stack_insert(bitcoinconsensus_script_execution_t *, bitcoinconsensus_script_execution_stack, int stackPos, const void *data, size_t dataLen, bitcoinconsensus_error*);

/// Deletes the element at a given position of one of the stacks.
/// A negative stackPos will insert at the end of the stack.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_stack_delete(bitcoinconsensus_script_execution_t *, bitcoinconsensus_script_execution_stack, int stackPos, bitcoinconsensus_error*);

/// Accesses the element at a given position of one of the stacks.
/// A negative stackPos will insert at the end of the stack.
/// When successful (indicated by a non-zero return value), data will receive a pointer to the stack element, and dataLen will be set to its size.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_stack_get(bitcoinconsensus_script_execution_t *, bitcoinconsensus_script_execution_stack, int stackPos, const void **data, size_t *dataLen, bitcoinconsensus_error*);

/// Starts execution of the script.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_start(bitcoinconsensus_script_execution_t *, unsigned int flags, bitcoinconsensus_error*);

/// Executes a single instruction of the script.
/// When successful, fEof will be assigned to false if there is more to do, or true when the script has completed.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_step(bitcoinconsensus_script_execution_t *, int *fEof, bitcoinconsensus_error*);

/// Immediately terminates the execution.
/// Unless bitcoinconsensus_script_execution_start returned true, this will crash your program.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_terminate(bitcoinconsensus_script_execution_t *, bitcoinconsensus_error*);

/// Returns the current execution offset in the script.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_get_pc(bitcoinconsensus_script_execution_t *, bitcoinconsensus_error*);

/// Returns the position in the script following the last OP_CODESEPARATOR executed, or zero if none have executed.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_get_codehash_pos(bitcoinconsensus_script_execution_t *, bitcoinconsensus_error*);

/// Returns the number of "sigops" executed thus far.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_script_execution_get_sigop_count(bitcoinconsensus_script_execution_t *, bitcoinconsensus_error*);

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not NULL, err will contain an error/success code for the operation
EXPORT_SYMBOL int bitcoinconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, bitcoinconsensus_error* err);

EXPORT_SYMBOL unsigned int bitcoinconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // BITCOIN_BITCOINCONSENSUS_H
