// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_RPCSERVER_H
#define SYSCOIN_RPCSERVER_H

#include "amount.h"
#include "rpc/protocol.h"
#include "uint256.h"

#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include <boost/function.hpp>

#include <univalue.h>

class CRPCCommand;

namespace RPCServer
{
    void OnStarted(boost::function<void ()> slot);
    void OnStopped(boost::function<void ()> slot);
    void OnPreCommand(boost::function<void (const CRPCCommand&)> slot);
    void OnPostCommand(boost::function<void (const CRPCCommand&)> slot);
}

class CBlockIndex;
class CNetAddr;

/** Wrapper for UniValue::VType, which includes typeAny:
 * Used to denote don't care type. Only used by RPCTypeCheckObj */
struct UniValueType {
    UniValueType(UniValue::VType _type) : typeAny(false), type(_type) {}
    UniValueType() : typeAny(true) {}
    bool typeAny;
    UniValue::VType type;
};

class JSONRPCRequest
{
public:
    UniValue id;
    std::string strMethod;
    UniValue params;
    bool fHelp;
    std::string URI;
    std::string authUser;

    JSONRPCRequest() { id = NullUniValue; params = NullUniValue; fHelp = false; }
    void parse(const UniValue& valRequest);
};

/** Query whether RPC is running */
bool IsRPCRunning();

/**
 * Set the RPC warmup status.  When this is done, all RPC calls will error out
 * immediately with RPC_IN_WARMUP.
 */
void SetRPCWarmupStatus(const std::string& newStatus);
/* Mark warmup as done.  RPC calls will be processed from now on.  */
void SetRPCWarmupFinished();

/* returns the current warmup state.  */
bool RPCIsInWarmup(std::string *statusOut);

/**
 * Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
 * the right number of arguments are passed, just that any passed are the correct type.
 */
void RPCTypeCheck(const UniValue& params,
                  const std::list<UniValue::VType>& typesExpected, bool fAllowNull=false);

/**
 * Type-check one argument; throws JSONRPCError if wrong type given.
 */
void RPCTypeCheckArgument(const UniValue& value, UniValue::VType typeExpected);

/*
  Check for expected keys/value types in an Object.
*/
void RPCTypeCheckObj(const UniValue& o,
    const std::map<std::string, UniValueType>& typesExpected,
    bool fAllowNull = false,
    bool fStrict = false);

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase
{
public:
    virtual ~RPCTimerBase() {}
};

/**
 * RPC timer "driver".
 */
class RPCTimerInterface
{
public:
    virtual ~RPCTimerInterface() {}
    /** Implementation name */
    virtual const char *Name() = 0;
    /** Factory function for timers.
     * RPC will call the function to create a timer that will call func in *millis* milliseconds.
     * @note As the RPC mechanism is backend-neutral, it can use different implementations of timers.
     * This is needed to cope with the case in which there is no HTTP server, but
     * only GUI RPC console, and to break the dependency of pcserver on httprpc.
     */
    virtual RPCTimerBase* NewTimer(boost::function<void(void)>& func, int64_t millis) = 0;
};

/** Set the factory function for timers */
void RPCSetTimerInterface(RPCTimerInterface *iface);
/** Set the factory function for timer, but only, if unset */
void RPCSetTimerInterfaceIfUnset(RPCTimerInterface *iface);
/** Unset factory function for timers */
void RPCUnsetTimerInterface(RPCTimerInterface *iface);

/**
 * Run func nSeconds from now.
 * Overrides previous timer <name> (if any).
 */
void RPCRunLater(const std::string& name, boost::function<void(void)> func, int64_t nSeconds);

typedef UniValue(*rpcfn_type)(const JSONRPCRequest& jsonRequest);

class CRPCCommand
{
public:
    std::string category;
    std::string name;
    rpcfn_type actor;
    bool okSafeMode;
    std::vector<std::string> argNames;
};

/**
 * Syscoin RPC command dispatcher.
 */
class CRPCTable
{
private:
    std::map<std::string, const CRPCCommand*> mapCommands;
public:
    CRPCTable();
    const CRPCCommand* operator[](const std::string& name) const;
    std::string help(const std::string& name) const;

    /**
     * Execute a method.
     * @param request The JSONRPCRequest to execute
     * @returns Result of the call.
     * @throws an exception (UniValue) when an error happens.
     */
    UniValue execute(const JSONRPCRequest &request) const;

    /**
    * Returns a list of registered commands
    * @returns List of registered commands.
    */
    std::vector<std::string> listCommands() const;

    /**
     * Appends a CRPCCommand to the dispatch table.
     * Returns false if RPC server is already running (dump concurrency protection).
     * Commands cannot be overwritten (returns false).
     */
    bool appendCommand(const std::string& name, const CRPCCommand* pcmd);
};

extern CRPCTable tableRPC;
// SYSCOIN service rpc functions
extern UniValue aliasnew(const JSONRPCRequest& request);
extern UniValue syscointxfund(const JSONRPCRequest& request);

extern UniValue aliasupdate(const JSONRPCRequest& request);
extern UniValue aliasinfo(const JSONRPCRequest& request);
extern UniValue aliasbalance(const JSONRPCRequest& request);
extern UniValue prunesyscoinservices(const JSONRPCRequest& request);
extern UniValue aliaspay(const JSONRPCRequest& request);
extern UniValue aliasaddscript(const JSONRPCRequest& request);
extern UniValue aliasupdatewhitelist(const JSONRPCRequest& request);
extern UniValue aliasclearwhitelist(const JSONRPCRequest& request);
extern UniValue aliaswhitelist(const JSONRPCRequest& request);
extern UniValue syscoinlistreceivedbyaddress(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
extern UniValue createrawtransaction(const JSONRPCRequest& request);
extern UniValue syscoinsendrawtransaction(const JSONRPCRequest& request);
extern UniValue syscoindecoderawtransaction(const JSONRPCRequest& request);
extern UniValue offernew(const JSONRPCRequest& request);
extern UniValue offerupdate(const JSONRPCRequest& request);
extern UniValue offerlink(const JSONRPCRequest& request);
extern UniValue offerinfo(const JSONRPCRequest& request);

extern UniValue certupdate(const JSONRPCRequest& request);
extern UniValue certnew(const JSONRPCRequest& request);
extern UniValue certtransfer(const JSONRPCRequest& request);
extern UniValue certinfo(const JSONRPCRequest& request);

extern UniValue escrownew(const JSONRPCRequest& request);
extern UniValue escrowbid(const JSONRPCRequest& request);
extern UniValue escrowcreaterawtransaction(const JSONRPCRequest& request);
extern UniValue escrowrelease(const JSONRPCRequest& request);
extern UniValue escrowcompleterelease(const JSONRPCRequest& request);
extern UniValue escrowrefund(const JSONRPCRequest& request);
extern UniValue escrowcompleterefund(const JSONRPCRequest& request);
extern UniValue escrowinfo(const JSONRPCRequest& request);
extern UniValue escrowfeedback(const JSONRPCRequest& request);
extern UniValue escrowacknowledge(const JSONRPCRequest& request);
extern UniValue createmultisig(const JSONRPCRequest& request);

extern UniValue assetnew(const JSONRPCRequest& request);
extern UniValue assetupdate(const JSONRPCRequest& request);
extern UniValue assettransfer(const JSONRPCRequest& request);
extern UniValue assetsend(const JSONRPCRequest& request);
extern UniValue assetinfo(const JSONRPCRequest& request);
extern UniValue assetallocationsend(const JSONRPCRequest& request);
extern UniValue assetallocationcollectinterest(const JSONRPCRequest& request);
extern UniValue assetallocationinfo(const JSONRPCRequest& request);
extern UniValue assetallocationsenderstatus(const JSONRPCRequest& request);
/**
 * Utilities: convert hex-encoded Values
 * (throws error if not hex).
 */
extern uint256 ParseHashV(const UniValue& v, std::string strName);
extern uint256 ParseHashO(const UniValue& o, std::string strKey);
extern std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName);
extern std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey);

extern int64_t nWalletUnlockTime;
extern CAmount AmountFromValue(const UniValue& value);
extern UniValue ValueFromAmount(const CAmount& amount);
extern double GetDifficulty(const CBlockIndex* blockindex = NULL);
extern std::string HelpRequiringPassphrase();
extern std::string HelpExampleCli(const std::string& methodname, const std::string& args);
extern std::string HelpExampleRpc(const std::string& methodname, const std::string& args);

extern void EnsureWalletIsUnlocked();

extern UniValue tpstestinfo(const UniValue& params, bool fHelp);
bool StartRPC();
void InterruptRPC();
void StopRPC();
std::string JSONRPCExecBatch(const UniValue& vReq);
void RPCNotifyBlockChange(bool ibd, const CBlockIndex *);

#endif // SYSCOIN_RPCSERVER_H

