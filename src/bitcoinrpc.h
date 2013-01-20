// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINRPC_H_
#define _BITCOINRPC_H_ 1

#include <string>
#include <list>
#include <map>

class CBlockIndex;

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include "util.h"

// HTTP status codes
enum HTTPStatusCode
{
    HTTP_OK                    = 200,
    HTTP_BAD_REQUEST           = 400,
    HTTP_UNAUTHORIZED          = 401,
    HTTP_FORBIDDEN             = 403,
    HTTP_NOT_FOUND             = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500,
};

// Bitcoin RPC error codes
enum RPCErrorCode
{
    // Standard JSON-RPC 2.0 errors
    RPC_INVALID_REQUEST  = -32600,
    RPC_METHOD_NOT_FOUND = -32601,
    RPC_INVALID_PARAMS   = -32602,
    RPC_INTERNAL_ERROR   = -32603,
    RPC_PARSE_ERROR      = -32700,

    // General application defined errors
    RPC_MISC_ERROR                  = -1,  // std::exception thrown in command handling
    RPC_FORBIDDEN_BY_SAFE_MODE      = -2,  // Server is in safe mode, and command is not allowed in safe mode
    RPC_TYPE_ERROR                  = -3,  // Unexpected type was passed as parameter
    RPC_INVALID_ADDRESS_OR_KEY      = -5,  // Invalid address or key
    RPC_OUT_OF_MEMORY               = -7,  // Ran out of memory during operation
    RPC_INVALID_PARAMETER           = -8,  // Invalid, missing or duplicate parameter
    RPC_DATABASE_ERROR              = -20, // Database error
    RPC_DESERIALIZATION_ERROR       = -22, // Error parsing or validating structure in raw format

    // P2P client errors
    RPC_CLIENT_NOT_CONNECTED        = -9,  // Bitcoin is not connected
    RPC_CLIENT_IN_INITIAL_DOWNLOAD  = -10, // Still downloading initial blocks

    // Wallet errors
    RPC_WALLET_ERROR                = -4,  // Unspecified problem with wallet (key not found etc.)
    RPC_WALLET_INSUFFICIENT_FUNDS   = -6,  // Not enough funds in wallet or account
    RPC_WALLET_INVALID_ACCOUNT_NAME = -11, // Invalid account name
    RPC_WALLET_KEYPOOL_RAN_OUT      = -12, // Keypool ran out, call keypoolrefill first
    RPC_WALLET_UNLOCK_NEEDED        = -13, // Enter the wallet passphrase with walletpassphrase first
    RPC_WALLET_PASSPHRASE_INCORRECT = -14, // The wallet passphrase entered was incorrect
    RPC_WALLET_WRONG_ENC_STATE      = -15, // Command given in wrong wallet encryption state (encrypting an encrypted wallet etc.)
    RPC_WALLET_ENCRYPTION_FAILED    = -16, // Failed to encrypt the wallet
    RPC_WALLET_ALREADY_UNLOCKED     = -17, // Wallet is already unlocked
};

json_spirit::Object JSONRPCError(int code, const std::string& message);

void ThreadRPCServer(void* parg);
int CommandLineRPC(int argc, char *argv[]);

/** Convert parameter values for RPC call from strings to command-specific JSON objects. */
json_spirit::Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams);

/*
  Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
  the right number of arguments are passed, just that any passed are the correct type.
  Use like:  RPCTypeCheck(params, boost::assign::list_of(str_type)(int_type)(obj_type));
*/
void RPCTypeCheck(const json_spirit::Array& params,
                  const std::list<json_spirit::Value_type>& typesExpected, bool fAllowNull=false);
/*
  Check for expected keys/value types in an Object.
  Use like: RPCTypeCheck(object, boost::assign::map_list_of("name", str_type)("value", int_type));
*/
void RPCTypeCheck(const json_spirit::Object& o,
                  const std::map<std::string, json_spirit::Value_type>& typesExpected, bool fAllowNull=false);

class CWallet;
typedef json_spirit::Value(*rpcfn_type)(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

class CRPCCommand
{
public:
    std::string name;
    rpcfn_type actor;
    bool okSafeMode;
    bool unlocked;
    bool isWalletFn;
};

/**
 * Bitcoin RPC command dispatcher.
 */
class CRPCTable
{
private:
    std::map<std::string, const CRPCCommand*> mapCommands;
public:
    CRPCTable();
    const CRPCCommand* operator[](std::string name) const;
    std::string help(std::string name) const;

    /**
     * Execute a method.
     * @param method   Method to execute
     * @param params   Array of arguments (JSON objects)
     * @returns Result of the call.
     * @throws an exception (json_spirit::Value) when an error happens.
     */
    json_spirit::Value execute(const std::string &method, const json_spirit::Array &params, CWallet* pWallet = NULL) const;
};

extern const CRPCTable tableRPC;

extern int64 nWalletUnlockTime;
extern int64 AmountFromValue(const json_spirit::Value& value);
extern json_spirit::Value ValueFromAmount(int64 amount);
extern double GetDifficulty(const CBlockIndex* blockindex = NULL);
extern std::string HexBits(unsigned int nBits);
extern std::string HelpRequiringPassphrase(CWallet* pWallet = NULL);
extern void EnsureWalletIsUnlocked(CWallet* pWallet = NULL);

extern json_spirit::Value getconnectioncount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rpcnet.cpp
extern json_spirit::Value getpeerinfo(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value dumpprivkey(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rpcdump.cpp
extern json_spirit::Value importprivkey(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getgenerate(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rpcmining.cpp
extern json_spirit::Value setgenerate(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gethashespersec(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getmininginfo(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getwork(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblocktemplate(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitblock(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getnewaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rpcwallet.cpp
extern json_spirit::Value getaccountaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setaccount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaccount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaddressesbyaccount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendtoaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signmessage(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifymessage(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaccount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getbalance(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value movecmd(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendfrom(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendmany(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addmultisigaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createmultisig(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listreceivedbyaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listreceivedbyaccount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listtransactions(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listaddressgroupings(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listaccounts(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listsinceblock(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value backupwallet(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value keypoolrefill(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listwallets(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value usewallet(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value loadwallet(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value unloadwallet(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrase(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrasechange(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletlock(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value encryptwallet(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value validateaddress(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getinfo(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getrawtransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rcprawtransaction.cpp
extern json_spirit::Value listunspent(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value lockunspent(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listlockunspent(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createrawtransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decoderawtransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signrawtransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendrawtransaction(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getblockcount(CWallet* pWallet, const json_spirit::Array& params, bool fHelp); // in rpcblockchain.cpp
extern json_spirit::Value getdifficulty(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value settxfee(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getrawmempool(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockhash(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblock(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettxoutsetinfo(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettxout(CWallet* pWallet, const json_spirit::Array& params, bool fHelp);

#endif
