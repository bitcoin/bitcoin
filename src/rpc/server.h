// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_SERVER_H
#define BITCOIN_RPC_SERVER_H

#include <rpc/request.h>
#include <rpc/util.h>

#include <functional>
#include <map>
#include <stdint.h>
#include <string>

#include <univalue.h>

class CRPCCommand;

/** Query whether RPC is running */
bool IsRPCRunning();

/** Throw JSONRPCError if RPC is not running */
void RpcInterruptionPoint();

/**
 * Set the RPC warmup status.  When this is done, all RPC calls will error out
 * immediately with RPC_IN_WARMUP.
 */
void SetRPCWarmupStatus(const std::string& newStatus);
/* Mark warmup as done.  RPC calls will be processed from now on.  */
void SetRPCWarmupFinished();

/* returns the current warmup state.  */
bool RPCIsInWarmup(std::string *outStatus);

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase
{
public:
    virtual ~RPCTimerBase() = default;
};

/**
 * RPC timer "driver".
 */
class RPCTimerInterface
{
public:
    virtual ~RPCTimerInterface() = default;
    /** Implementation name */
    virtual const char *Name() = 0;
    /** Factory function for timers.
     * RPC will call the function to create a timer that will call func in *millis* milliseconds.
     * @note As the RPC mechanism is backend-neutral, it can use different implementations of timers.
     * This is needed to cope with the case in which there is no HTTP server, but
     * only GUI RPC console, and to break the dependency of pcserver on httprpc.
     */
    virtual RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) = 0;
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
void RPCRunLater(const std::string& name, std::function<void()> func, int64_t nSeconds);

typedef RPCHelpMan (*RpcMethodFnType)();

class CRPCCommand
{
public:
    //! RPC method handler reading request and assigning result. Should return
    //! true if request is fully handled, false if it should be passed on to
    //! subsequent handlers.
    using Actor = std::function<bool(const JSONRPCRequest& request, UniValue& result, bool last_handler)>;

    //! Constructor taking Actor callback supporting multiple handlers.
    CRPCCommand(std::string category, std::string name, Actor actor, std::vector<std::pair<std::string, bool>> args, intptr_t unique_id)
        : category(std::move(category)), name(std::move(name)), actor(std::move(actor)), argNames(std::move(args)),
          unique_id(unique_id)
    {
    }

    //! Simplified constructor taking plain RpcMethodFnType function pointer.
    CRPCCommand(std::string category, RpcMethodFnType fn)
        : CRPCCommand(
              category,
              fn().m_name,
              [fn](const JSONRPCRequest& request, UniValue& result, bool) { result = fn().HandleRequest(request); return true; },
              fn().GetArgNames(),
              intptr_t(fn))
    {
    }

    std::string category;
    std::string name;
    Actor actor;
    //! List of method arguments and whether they are named-only. Incoming RPC
    //! requests contain a "params" field that can either be an array containing
    //! unnamed arguments or an object containing named arguments. The
    //! "argNames" vector is used in the latter case to transform the params
    //! object into an array. Each argument in "argNames" gets mapped to a
    //! unique position in the array, based on the order it is listed, unless
    //! the argument is a named-only argument with argNames[x].second set to
    //! true. Named-only arguments are combined into a JSON object that is
    //! appended after other arguments, see transformNamedArguments for details.
    std::vector<std::pair<std::string, bool>> argNames;
    intptr_t unique_id;
};

/**
 * RPC command dispatcher.
 */
class CRPCTable
{
private:
    std::map<std::string, std::vector<const CRPCCommand*>> mapCommands;
public:
    CRPCTable();
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
     * Return all named arguments that need to be converted by the client from string to another JSON type
     */
    UniValue dumpArgMap(const JSONRPCRequest& request) const;

    /**
     * Appends a CRPCCommand to the dispatch table.
     *
     * Precondition: RPC server is not running
     *
     * Commands with different method names but the same unique_id will
     * be considered aliases, and only the first registered method name will
     * show up in the help text command listing. Aliased commands do not have
     * to have the same behavior. Server and client code can distinguish
     * between calls based on method name, and aliased commands can also
     * register different names, types, and numbers of parameters.
     */
    void appendCommand(const std::string& name, const CRPCCommand* pcmd);
    bool removeCommand(const std::string& name, const CRPCCommand* pcmd);
};

bool IsDeprecatedRPCEnabled(const std::string& method);

extern CRPCTable tableRPC;

void StartRPC();
void InterruptRPC();
void StopRPC();
UniValue JSONRPCExec(const JSONRPCRequest& jreq, bool catch_errors);

#endif // BITCOIN_RPC_SERVER_H
