// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCSERVER_H
#define BITCOIN_RPCSERVER_H

#include <amount.h>
#include <rpc/protocol.h>
#include <uint256.h>

#include <algorithm>
#include <list>
#include <map>
#include <stdint.h>
#include <string>

#include <univalue.h>

static const unsigned int DEFAULT_RPC_SERIALIZE_VERSION = 1;

static const std::string ARGS_WERE_POSITIONAL = "_positional";

class CRPCCommand;

namespace RPCServer
{
    void OnStarted(std::function<void ()> slot);
    void OnStopped(std::function<void ()> slot);
}

/** Set of allowed UniValue::VTypes. */
class UniValueType {
private:
    std::vector<bool> m_allowed_types;
    bool typeAny;

public:
    UniValueType(UniValue::VType _type) : typeAny(false) {
        auto type_int = unsigned(_type);
        m_allowed_types.resize(type_int + 1);
        m_allowed_types[type_int] = true;
    }
    UniValueType(std::vector<UniValue::VType> types) : typeAny(false) {
        unsigned int type_vector_size = 0;
        for (const auto type : types) {
            type_vector_size = std::max(type_vector_size, unsigned(type) + 1);
        }
        m_allowed_types.resize(type_vector_size);
        for (const auto type : types) {
            m_allowed_types[unsigned(type)] = true;
        }
    }
    UniValueType(std::initializer_list<UniValue::VType> types) : UniValueType(std::vector<UniValue::VType>(types)) { }
    UniValueType() : typeAny(true) {}

    bool IsAllowed(UniValue::VType type) const {
        if (typeAny) return true;
        auto type_int = unsigned(type);
        if (type_int >= m_allowed_types.size()) {
            return false;
        }
        return m_allowed_types[type_int];
    }

    // NOTE: Only valid if not typeAny! Will abort otherwise.
    std::vector<UniValue::VType> AllAllowedTypes() const {
        assert(!typeAny);
        std::vector<UniValue::VType> result;
        for (size_t i = 0; i < m_allowed_types.size(); ++i) {
            if (m_allowed_types[i]) {
                result.push_back(UniValue::VType(i));
            }
        }
        return result;
    }

    std::string DescribeAllowedTypes() const {
        if (typeAny) return "any";
        std::string s;
        for (const auto type : AllAllowedTypes()) {
            if (!s.empty()) {
                s += " or ";
            }
            s += uvTypeName(type);
        }
        return s;
    }
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
    std::string peerAddr;

    JSONRPCRequest() : id(NullUniValue), params(NullUniValue), fHelp(false) {}
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
bool RPCIsInWarmup(std::string *outStatus);

/**
 * Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
 * the right number of arguments are passed, just that any passed are the correct type.
 */
void RPCTypeCheck(const UniValue& params,
                  const std::list<UniValueType>& typesExpected, bool fAllowNull=false);

/**
 * Type-check one argument; throws JSONRPCError if wrong type given.
 */
void RPCTypeCheckArgument(const UniValue& value, const UniValueType& typeExpected);

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
    virtual RPCTimerBase* NewTimer(std::function<void(void)>& func, int64_t millis) = 0;
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
void RPCRunLater(const std::string& name, std::function<void(void)> func, int64_t nSeconds);

typedef UniValue(*rpcfn_type)(const JSONRPCRequest& jsonRequest);

class CRPCCommand
{
public:
    std::string category;
    std::string name;
    rpcfn_type actor;
    std::vector<std::string> argNames;
    bool named_args;
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
    const CRPCCommand* operator[](const std::string& name) const;
    std::string help(const std::string& name, const JSONRPCRequest& helpreq) const;

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
     *
     * Returns false if RPC server is already running (dump concurrency protection).
     *
     * Commands cannot be overwritten (returns false).
     *
     * Commands with different method names but the same callback function will
     * be considered aliases, and only the first registered method name will
     * show up in the help text command listing. Aliased commands do not have
     * to have the same behavior. Server and client code can distinguish
     * between calls based on method name, and aliased commands can also
     * register different names, types, and numbers of parameters.
     */
    bool appendCommand(const std::string& name, const CRPCCommand* pcmd);
};

bool IsDeprecatedRPCEnabled(const std::string& method);

extern CRPCTable tableRPC;

/**
 * Utilities: convert hex-encoded Values
 * (throws error if not hex).
 */
extern uint256 ParseHashV(const UniValue& v, std::string strName);
extern uint256 ParseHashO(const UniValue& o, std::string strKey);
extern std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName);
extern std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey);

extern CAmount AmountFromValue(const UniValue& value);
extern std::string HelpExampleCli(const std::string& methodname, const std::string& args);
extern std::string HelpExampleRpc(const std::string& methodname, const std::string& args);

bool StartRPC();
void InterruptRPC();
void StopRPC();
std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const UniValue& vReq);

// Retrieves any serialization flags requested in command line argument
int RPCSerializationFlags();

#endif // BITCOIN_RPCSERVER_H
