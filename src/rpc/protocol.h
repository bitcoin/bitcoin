// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_RPC_PROTOCOL_H
#define TORTOISECOIN_RPC_PROTOCOL_H

//! HTTP status codes
enum HTTPStatusCode
{
    HTTP_OK                    = 200,
    HTTP_NO_CONTENT            = 204,
    HTTP_BAD_REQUEST           = 400,
    HTTP_UNAUTHORIZED          = 401,
    HTTP_FORBIDDEN             = 403,
    HTTP_NOT_FOUND             = 404,
    HTTP_BAD_METHOD            = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_SERVICE_UNAVAILABLE   = 503,
};

//! Tortoisecoin RPC error codes
enum RPCErrorCode
{
    //! Standard JSON-RPC 2.0 errors
    // RPC_INVALID_REQUEST is internally mapped to HTTP_BAD_REQUEST (400).
    // It should not be used for application-layer errors.
    RPC_INVALID_REQUEST  = -32600,
    // RPC_METHOD_NOT_FOUND is internally mapped to HTTP_NOT_FOUND (404).
    // It should not be used for application-layer errors.
    RPC_METHOD_NOT_FOUND = -32601,
    RPC_INVALID_PARAMS   = -32602,
    // RPC_INTERNAL_ERROR should only be used for genuine errors in tortoisecoind
    // (for example datadir corruption).
    RPC_INTERNAL_ERROR   = -32603,
    RPC_PARSE_ERROR      = -32700,

    //! General application defined errors
    RPC_MISC_ERROR                  = -1,  //!< std::exception thrown in command handling
    RPC_TYPE_ERROR                  = -3,  //!< Unexpected type was passed as parameter
    RPC_INVALID_ADDRESS_OR_KEY      = -5,  //!< Invalid address or key
    RPC_OUT_OF_MEMORY               = -7,  //!< Ran out of memory during operation
    RPC_INVALID_PARAMETER           = -8,  //!< Invalid, missing or duplicate parameter
    RPC_DATABASE_ERROR              = -20, //!< Database error
    RPC_DESERIALIZATION_ERROR       = -22, //!< Error parsing or validating structure in raw format
    RPC_VERIFY_ERROR                = -25, //!< General error during transaction or block submission
    RPC_VERIFY_REJECTED             = -26, //!< Transaction or block was rejected by network rules
    RPC_VERIFY_ALREADY_IN_UTXO_SET  = -27, //!< Transaction already in utxo set
    RPC_IN_WARMUP                   = -28, //!< Client still warming up
    RPC_METHOD_DEPRECATED           = -32, //!< RPC method is deprecated

    //! Aliases for backward compatibility
    RPC_TRANSACTION_ERROR           = RPC_VERIFY_ERROR,
    RPC_TRANSACTION_REJECTED        = RPC_VERIFY_REJECTED,

    //! P2P client errors
    RPC_CLIENT_NOT_CONNECTED        = -9,  //!< Tortoisecoin is not connected
    RPC_CLIENT_IN_INITIAL_DOWNLOAD  = -10, //!< Still downloading initial blocks
    RPC_CLIENT_NODE_ALREADY_ADDED   = -23, //!< Node is already added
    RPC_CLIENT_NODE_NOT_ADDED       = -24, //!< Node has not been added before
    RPC_CLIENT_NODE_NOT_CONNECTED   = -29, //!< Node to disconnect not found in connected nodes
    RPC_CLIENT_INVALID_IP_OR_SUBNET = -30, //!< Invalid IP/Subnet
    RPC_CLIENT_P2P_DISABLED         = -31, //!< No valid connection manager instance found
    RPC_CLIENT_NODE_CAPACITY_REACHED= -34, //!< Max number of outbound or block-relay connections already open

    //! Chain errors
    RPC_CLIENT_MEMPOOL_DISABLED     = -33, //!< No mempool instance found

    //! Wallet errors
    RPC_WALLET_ERROR                = -4,  //!< Unspecified problem with wallet (key not found etc.)
    RPC_WALLET_INSUFFICIENT_FUNDS   = -6,  //!< Not enough funds in wallet or account
    RPC_WALLET_INVALID_LABEL_NAME   = -11, //!< Invalid label name
    RPC_WALLET_KEYPOOL_RAN_OUT      = -12, //!< Keypool ran out, call keypoolrefill first
    RPC_WALLET_UNLOCK_NEEDED        = -13, //!< Enter the wallet passphrase with walletpassphrase first
    RPC_WALLET_PASSPHRASE_INCORRECT = -14, //!< The wallet passphrase entered was incorrect
    RPC_WALLET_WRONG_ENC_STATE      = -15, //!< Command given in wrong wallet encryption state (encrypting an encrypted wallet etc.)
    RPC_WALLET_ENCRYPTION_FAILED    = -16, //!< Failed to encrypt the wallet
    RPC_WALLET_ALREADY_UNLOCKED     = -17, //!< Wallet is already unlocked
    RPC_WALLET_NOT_FOUND            = -18, //!< Invalid wallet specified
    RPC_WALLET_NOT_SPECIFIED        = -19, //!< No wallet specified (error when there are multiple wallets loaded)
    RPC_WALLET_ALREADY_LOADED       = -35, //!< This same wallet is already loaded
    RPC_WALLET_ALREADY_EXISTS       = -36, //!< There is already a wallet with the same name

    //! Backwards compatible aliases
    RPC_WALLET_INVALID_ACCOUNT_NAME = RPC_WALLET_INVALID_LABEL_NAME,

    //! Unused reserved codes, kept around for backwards compatibility. Do not reuse.
    RPC_FORBIDDEN_BY_SAFE_MODE      = -2,  //!< Server is in safe mode, and command is not allowed in safe mode
};

#endif // TORTOISECOIN_RPC_PROTOCOL_H
