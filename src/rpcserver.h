// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITCOINRPC_SERVER_H_
#define _BITCOINRPC_SERVER_H_ 1

#include "uint256.h"
#include "rpcprotocol.h"

#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

class CBlockIndexBase;
class CNetAddr;

/* Start RPC threads */
void StartRPCThreads();
/* Alternative to StartRPCThreads for the GUI, when no server is
 * used. The RPC thread in this case is only used to handle timeouts.
 * If real RPC threads have already been started this is a no-op.
 */
void StartDummyRPCThread();
/* Stop RPC threads */
void StopRPCThreads();

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

/*
  Run func nSeconds from now. Uses boost deadline timers.
  Overrides previous timer <name> (if any).
 */
void RPCRunLater(const std::string& name, boost::function<void(void)> func, int64_t nSeconds);

//! Convert boost::asio address to CNetAddr
extern CNetAddr BoostAsioToCNetAddr(boost::asio::ip::address address);

typedef json_spirit::Value(*rpcfn_type)(const json_spirit::Array& params, bool fHelp);

class CRPCCommand
{
public:
    std::string name;
    rpcfn_type actor;
    bool okSafeMode;
    bool threadSafe;
    bool reqWallet;
};

/**
 * Credits RPC command dispatcher.
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
    json_spirit::Value execute(const std::string &method, const json_spirit::Array &params) const;
};

extern const CRPCTable tableRPC;

//
// Utilities: convert hex-encoded Values
// (throws error if not hex).
//
extern uint256 ParseHashV(const json_spirit::Value& v, std::string strName);
extern uint256 ParseHashO(const json_spirit::Object& o, std::string strKey);
extern std::vector<unsigned char> ParseHexV(const json_spirit::Value& v, std::string strName);
extern std::vector<unsigned char> ParseHexO(const json_spirit::Object& o, std::string strKey);

extern void InitRPCMining(bool coinbaseDepositDisabled);
extern void ShutdownRPCMining();

extern int64_t bitcredit_nWalletUnlockTime;
extern int64_t deposit_nWalletUnlockTime;
extern int64_t bitcoin_nWalletUnlockTime;
extern int64_t Credits_AmountFromValue(const json_spirit::Value& value);
extern int64_t Bitcoin_AmountFromValue(const json_spirit::Value& value);
extern json_spirit::Value ValueFromAmount(int64_t amount);
extern double Bitcredit_GetDifficulty(const CBlockIndexBase* blockindex = NULL);
extern double Bitcoin_GetDifficulty(const CBlockIndexBase* blockindex = NULL);
extern std::string HexBits(unsigned int nBits);
extern std::string HelpRequiringPassphrase();
extern std::string HelpExampleCli(std::string methodname, std::string args);
extern std::string HelpExampleRpc(std::string methodname, std::string args);

extern void Bitcredit_EnsureWalletIsUnlocked();
extern void Bitcoin_EnsureWalletIsUnlocked();

extern json_spirit::Value bitcredit_getconnectioncount(const json_spirit::Array& params, bool fHelp); // in rpcnet.cpp
extern json_spirit::Value bitcoin_getconnectioncount(const json_spirit::Array& params, bool fHelp); // in rpcnet.cpp
extern json_spirit::Value bitcredit_getpeerinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getpeerinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_ping(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_ping(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_addnode(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_addnode(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getaddednodeinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getaddednodeinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getnettotals(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getnettotals(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value bitcredit_dumpprivkey(const json_spirit::Array& params, bool fHelp); // in rpcdump.cpp
extern json_spirit::Value bitcoin_dumpprivkey(const json_spirit::Array& params, bool fHelp); // in rpcdump.cpp
extern json_spirit::Value bitcredit_importprivkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_importprivkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_dumpwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_dumpwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_importwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_importwallet(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getgenerate(const json_spirit::Array& params, bool fHelp); // in rpcmining.cpp
extern json_spirit::Value setgenerate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getnetworkhashps(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gethashespersec(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getmininginfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getwork(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblocktemplate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitblock(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp); // in rpcwallet.cpp
extern json_spirit::Value getaccountaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getrawchangeaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaddressesbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_sendtoaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_sendtoaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signmessage(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifymessage(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getbalance(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getunconfirmedbalance(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value movecmd(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendfrom(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendmany(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addmultisigaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createmultisig(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_listreceivedbyaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_listreceivedbyaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_listreceivedbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_listreceivedbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_listtransactions(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_listtransactions(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_listaddressgroupings(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_listaddressgroupings(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listaccounts(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listsinceblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value backupwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value keypoolrefill(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deposit_walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deposit_walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_walletlock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deposit_walletlock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_walletlock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_encryptwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deposit_encryptwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_encryptwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value credits_validateaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_validateaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getwalletinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getwalletinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getblockchaininfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getblockchaininfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getnetworkinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getnetworkinfo(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getrawtransaction(const json_spirit::Array& params, bool fHelp); // in rcprawtransaction.cpp
extern json_spirit::Value listunspent(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value lockunspent(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listlockunspent(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createrawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decoderawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decodescript(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signrawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendrawtransaction(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value bitcredit_getblockcount(const json_spirit::Array& params, bool fHelp); // in rpcblockchain.cpp
extern json_spirit::Value bitcoin_getblockcount(const json_spirit::Array& params, bool fHelp); // in rpcblockchain.cpp
extern json_spirit::Value bitcredit_getbestblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getbestblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcredit_getdifficulty(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value bitcoin_getdifficulty(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value settxfee(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getrawmempool(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettxoutsetinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettxout(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifychain(const json_spirit::Array& params, bool fHelp);

#endif
