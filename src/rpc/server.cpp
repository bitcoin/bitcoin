// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>

#include <common/args.h>
#include <common/system.h>
#include <httprpc.h>
#include <logging.h>
#include <rpc/util.h>
#include <shutdown.h>
#include <sync.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>

#include <boost/signals2/signal.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

static GlobalMutex g_rpc_warmup_mutex;
static std::atomic<bool> g_rpc_running{false};
static bool fRPCInWarmup GUARDED_BY(g_rpc_warmup_mutex) = true;
static std::string rpcWarmupStatus GUARDED_BY(g_rpc_warmup_mutex) = "RPC server started";
/* Timer-creating functions */
static RPCTimerInterface* timerInterface = nullptr;
/* Map of name to timer. */
static GlobalMutex g_deadline_timers_mutex;
static std::map<std::string, std::unique_ptr<RPCTimerBase> > deadlineTimers GUARDED_BY(g_deadline_timers_mutex);
static bool ExecuteCommand(const CRPCCommand& command, const JSONRPCRequest& request, UniValue& result, bool last_handler);

struct RPCCommandExecutionInfo
{
    std::string method;
    SteadyClock::time_point start;
};

struct RPCServerInfo
{
    Mutex mutex;
    std::list<RPCCommandExecutionInfo> active_commands GUARDED_BY(mutex);
};

static RPCServerInfo g_rpc_server_info;

struct RPCCommandExecution
{
    std::list<RPCCommandExecutionInfo>::iterator it;
    explicit RPCCommandExecution(const std::string& method)
    {
        LOCK(g_rpc_server_info.mutex);
        it = g_rpc_server_info.active_commands.insert(g_rpc_server_info.active_commands.end(), {method, SteadyClock::now()});
    }
    ~RPCCommandExecution()
    {
        LOCK(g_rpc_server_info.mutex);
        g_rpc_server_info.active_commands.erase(it);
    }
};

static struct CRPCSignals
{
    boost::signals2::signal<void ()> Started;
    boost::signals2::signal<void ()> Stopped;
} g_rpcSignals;

void RPCServer::OnStarted(std::function<void ()> slot)
{
    g_rpcSignals.Started.connect(slot);
}

void RPCServer::OnStopped(std::function<void ()> slot)
{
    g_rpcSignals.Stopped.connect(slot);
}

std::string CRPCTable::help(const std::string& strCommand, const JSONRPCRequest& helpreq) const
{
    std::string strRet;
    std::string category;
    std::set<intptr_t> setDone;
    std::vector<std::pair<std::string, const CRPCCommand*> > vCommands;
    vCommands.reserve(mapCommands.size());

    for (const auto& entry : mapCommands)
        vCommands.emplace_back(entry.second.front()->category + entry.first, entry.second.front());
    sort(vCommands.begin(), vCommands.end());

    JSONRPCRequest jreq = helpreq;
    jreq.mode = JSONRPCRequest::GET_HELP;
    jreq.params = UniValue();

    for (const std::pair<std::string, const CRPCCommand*>& command : vCommands)
    {
        const CRPCCommand *pcmd = command.second;
        std::string strMethod = pcmd->name;
        if ((strCommand != "" || pcmd->category == "hidden") && strMethod != strCommand)
            continue;
        jreq.strMethod = strMethod;
        try
        {
            UniValue unused_result;
            if (setDone.insert(pcmd->unique_id).second)
                pcmd->actor(jreq, unused_result, /*last_handler=*/true);
        }
        catch (const std::exception& e)
        {
            // Help text is returned in an exception
            std::string strHelp = std::string(e.what());
            if (strCommand == "")
            {
                if (strHelp.find('\n') != std::string::npos)
                    strHelp = strHelp.substr(0, strHelp.find('\n'));

                if (category != pcmd->category)
                {
                    if (!category.empty())
                        strRet += "\n";
                    category = pcmd->category;
                    strRet += "== " + Capitalize(category) + " ==\n";
                }
            }
            strRet += strHelp + "\n";
        }
    }
    if (strRet == "")
        strRet = strprintf("help: unknown command: %s\n", strCommand);
    strRet = strRet.substr(0,strRet.size()-1);
    return strRet;
}

static RPCHelpMan help()
{
    return RPCHelpMan{"help",
                "\nList all commands, or get help for a specified command.\n",
                {
                    {"command", RPCArg::Type::STR, RPCArg::DefaultHint{"all commands"}, "The command to get help on"},
                },
                {
                    RPCResult{RPCResult::Type::STR, "", "The help text"},
                    RPCResult{RPCResult::Type::ANY, "", ""},
                },
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& jsonRequest) -> UniValue
{
    std::string strCommand;
    if (jsonRequest.params.size() > 0) {
        strCommand = jsonRequest.params[0].get_str();
    }
    if (strCommand == "dump_all_command_conversions") {
        // Used for testing only, undocumented
        return tableRPC.dumpArgMap(jsonRequest);
    }

    return tableRPC.help(strCommand, jsonRequest);
},
    };
}

static RPCHelpMan stop()
{
    static const std::string RESULT{PACKAGE_NAME " stopping"};
    return RPCHelpMan{"stop",
    // Also accept the hidden 'wait' integer argument (milliseconds)
    // For instance, 'stop 1000' makes the call wait 1 second before returning
    // to the client (intended for testing)
                "\nRequest a graceful shutdown of " PACKAGE_NAME ".",
                {
                    {"wait", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "how long to wait in ms", RPCArgOptions{.hidden=true}},
                },
                RPCResult{RPCResult::Type::STR, "", "A string with the content '" + RESULT + "'"},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& jsonRequest) -> UniValue
{
    // Event loop will exit after current HTTP requests have been handled, so
    // this reply will get back to the client.
    StartShutdown();
    if (jsonRequest.params[0].isNum()) {
        UninterruptibleSleep(std::chrono::milliseconds{jsonRequest.params[0].getInt<int>()});
    }
    return RESULT;
},
    };
}

static RPCHelpMan uptime()
{
    return RPCHelpMan{"uptime",
                "\nReturns the total uptime of the server.\n",
                            {},
                            RPCResult{
                                RPCResult::Type::NUM, "", "The number of seconds that the server has been running"
                            },
                RPCExamples{
                    HelpExampleCli("uptime", "")
                + HelpExampleRpc("uptime", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return GetTime() - GetStartupTime();
}
    };
}

static RPCHelpMan getrpcinfo()
{
    return RPCHelpMan{"getrpcinfo",
                "\nReturns details of the RPC server.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::ARR, "active_commands", "All active commands",
                        {
                            {RPCResult::Type::OBJ, "", "Information about an active command",
                            {
                                 {RPCResult::Type::STR, "method", "The name of the RPC command"},
                                 {RPCResult::Type::NUM, "duration", "The running time in microseconds"},
                            }},
                        }},
                        {RPCResult::Type::STR, "logpath", "The complete file path to the debug log"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getrpcinfo", "")
                + HelpExampleRpc("getrpcinfo", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    LOCK(g_rpc_server_info.mutex);
    UniValue active_commands(UniValue::VARR);
    for (const RPCCommandExecutionInfo& info : g_rpc_server_info.active_commands) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("method", info.method);
        entry.pushKV("duration", int64_t{Ticks<std::chrono::microseconds>(SteadyClock::now() - info.start)});
        active_commands.push_back(entry);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("active_commands", active_commands);

    const std::string path = LogInstance().m_file_path.u8string();
    UniValue log_path(UniValue::VSTR, path);
    result.pushKV("logpath", log_path);

    return result;
}
    };
}

static RPCHelpMan getrpcwhitelist()
{
    return RPCHelpMan{"getrpcwhitelist",
                "\nReturns whitelisted RPCs for the current user.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::OBJ_DYN, "methods", "List of RPCs that the user is allowed to call",
                        {
                            {RPCResult::Type::NONE, "rpc", "Key is name of RPC method, value is null"},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("getrpcwhitelist", "")
                + HelpExampleRpc("getrpcwhitelist", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue whitelisted_rpcs(UniValue::VOBJ);
    const std::set<std::string>& whitelist = GetWhitelistedRpcs(request.authUser);
    for (const auto& rpc : whitelist) {
        whitelisted_rpcs.pushKV(rpc, NullUniValue);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("methods", whitelisted_rpcs);

    return result;
}
    };
}

static const CRPCCommand vRPCCommands[]{
    /* Overall control/query calls */
    {"control", &getrpcinfo},
    {"control", &getrpcwhitelist},
    {"control", &help},
    {"control", &stop},
    {"control", &uptime},
};

CRPCTable::CRPCTable()
{
    for (const auto& c : vRPCCommands) {
        appendCommand(c.name, &c);
    }
}

void CRPCTable::appendCommand(const std::string& name, const CRPCCommand* pcmd)
{
    CHECK_NONFATAL(!IsRPCRunning()); // Only add commands before rpc is running

    mapCommands[name].push_back(pcmd);
}

bool CRPCTable::removeCommand(const std::string& name, const CRPCCommand* pcmd)
{
    auto it = mapCommands.find(name);
    if (it != mapCommands.end()) {
        auto new_end = std::remove(it->second.begin(), it->second.end(), pcmd);
        if (it->second.end() != new_end) {
            it->second.erase(new_end, it->second.end());
            return true;
        }
    }
    return false;
}

void StartRPC()
{
    LogPrint(BCLog::RPC, "Starting RPC\n");
    g_rpc_running = true;
    g_rpcSignals.Started();
}

void InterruptRPC()
{
    static std::once_flag g_rpc_interrupt_flag;
    // This function could be called twice if the GUI has been started with -server=1.
    std::call_once(g_rpc_interrupt_flag, []() {
        LogPrint(BCLog::RPC, "Interrupting RPC\n");
        // Interrupt e.g. running longpolls
        g_rpc_running = false;
    });
}

void StopRPC()
{
    static std::once_flag g_rpc_stop_flag;
    // This function could be called twice if the GUI has been started with -server=1.
    assert(!g_rpc_running);
    std::call_once(g_rpc_stop_flag, []() {
        LogPrint(BCLog::RPC, "Stopping RPC\n");
        WITH_LOCK(g_deadline_timers_mutex, deadlineTimers.clear());
        DeleteAuthCookie();
        g_rpcSignals.Stopped();
    });
}

bool IsRPCRunning()
{
    return g_rpc_running;
}

void RpcInterruptionPoint()
{
    if (!IsRPCRunning()) throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Shutting down");
}

void SetRPCWarmupStatus(const std::string& newStatus)
{
    LOCK(g_rpc_warmup_mutex);
    rpcWarmupStatus = newStatus;
}

void SetRPCWarmupFinished()
{
    LOCK(g_rpc_warmup_mutex);
    assert(fRPCInWarmup);
    fRPCInWarmup = false;
}

bool RPCIsInWarmup(std::string *outStatus)
{
    LOCK(g_rpc_warmup_mutex);
    if (outStatus)
        *outStatus = rpcWarmupStatus;
    return fRPCInWarmup;
}

bool IsDeprecatedRPCEnabled(const std::string& method)
{
    const std::vector<std::string> enabled_methods = gArgs.GetArgs("-deprecatedrpc");

    return find(enabled_methods.begin(), enabled_methods.end(), method) != enabled_methods.end();
}

static UniValue JSONRPCExecOne(JSONRPCRequest jreq, const UniValue& req)
{
    UniValue rpc_result(UniValue::VOBJ);

    try {
        jreq.parse(req);

        UniValue result = tableRPC.execute(jreq);
        rpc_result = JSONRPCReplyObj(result, NullUniValue, jreq.id);
    }
    catch (const UniValue& objError)
    {
        rpc_result = JSONRPCReplyObj(NullUniValue, objError, jreq.id);
    }
    catch (const std::exception& e)
    {
        rpc_result = JSONRPCReplyObj(NullUniValue,
                                     JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    }

    return rpc_result;
}

std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const UniValue& vReq)
{
    UniValue ret(UniValue::VARR);
    for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
        ret.push_back(JSONRPCExecOne(jreq, vReq[reqIdx]));

    return ret.write() + "\n";
}

/**
 * Process named arguments into a vector of positional arguments, based on the
 * passed-in specification for the RPC call's arguments.
 */
static inline JSONRPCRequest transformNamedArguments(const JSONRPCRequest& in, const std::vector<std::pair<std::string, bool>>& argNames)
{
    JSONRPCRequest out = in;
    out.params = UniValue(UniValue::VARR);
    // Build a map of parameters, and remove ones that have been processed, so that we can throw a focused error if
    // there is an unknown one.
    const std::vector<std::string>& keys = in.params.getKeys();
    const std::vector<UniValue>& values = in.params.getValues();
    std::unordered_map<std::string, const UniValue*> argsIn;
    for (size_t i=0; i<keys.size(); ++i) {
        auto [_, inserted] = argsIn.emplace(keys[i], &values[i]);
        if (!inserted) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter " + keys[i] + " specified multiple times");
        }
    }
    // Process expected parameters. If any parameters were left unspecified in
    // the request before a parameter that was specified, null values need to be
    // inserted at the unspecifed parameter positions, and the "hole" variable
    // below tracks the number of null values that need to be inserted.
    // The "initial_hole_size" variable stores the size of the initial hole,
    // i.e. how many initial positional arguments were left unspecified. This is
    // used after the for-loop to add initial positional arguments from the
    // "args" parameter, if present.
    int hole = 0;
    int initial_hole_size = 0;
    const std::string* initial_param = nullptr;
    UniValue options{UniValue::VOBJ};
    for (const auto& [argNamePattern, named_only]: argNames) {
        std::vector<std::string> vargNames = SplitString(argNamePattern, '|');
        auto fr = argsIn.end();
        for (const std::string & argName : vargNames) {
            fr = argsIn.find(argName);
            if (fr != argsIn.end()) {
                break;
            }
        }

        // Handle named-only parameters by pushing them into a temporary options
        // object, and then pushing the accumulated options as the next
        // positional argument.
        if (named_only) {
            if (fr != argsIn.end()) {
                if (options.exists(fr->first)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter " + fr->first + " specified multiple times");
                }
                options.pushKVEnd(fr->first, *fr->second);
                argsIn.erase(fr);
            }
            continue;
        }

        if (!options.empty() || fr != argsIn.end()) {
            for (int i = 0; i < hole; ++i) {
                // Fill hole between specified parameters with JSON nulls,
                // but not at the end (for backwards compatibility with calls
                // that act based on number of specified parameters).
                out.params.push_back(UniValue());
            }
            hole = 0;
            if (!initial_param) initial_param = &argNamePattern;
        } else {
            hole += 1;
            if (out.params.empty()) initial_hole_size = hole;
        }

        // If named input parameter "fr" is present, push it onto out.params. If
        // options are present, push them onto out.params. If both are present,
        // throw an error.
        if (fr != argsIn.end()) {
            if (!options.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter " + fr->first + " conflicts with parameter " + options.getKeys().front());
            }
            out.params.push_back(*fr->second);
            argsIn.erase(fr);
        }
        if (!options.empty()) {
            out.params.push_back(std::move(options));
            options = UniValue{UniValue::VOBJ};
        }
    }
    // If leftover "args" param was found, use it as a source of positional
    // arguments and add named arguments after. This is a convenience for
    // clients that want to pass a combination of named and positional
    // arguments as described in doc/JSON-RPC-interface.md#parameter-passing
    auto positional_args{argsIn.extract("args")};
    if (positional_args && positional_args.mapped()->isArray()) {
        if (initial_hole_size < (int)positional_args.mapped()->size() && initial_param) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter " + *initial_param + " specified twice both as positional and named argument");
        }
        // Assign positional_args to out.params and append named_args after.
        UniValue named_args{std::move(out.params)};
        out.params = *positional_args.mapped();
        for (size_t i{out.params.size()}; i < named_args.size(); ++i) {
            out.params.push_back(named_args[i]);
        }
    }
    // If there are still arguments in the argsIn map, this is an error.
    if (!argsIn.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown named parameter " + argsIn.begin()->first);
    }
    // Return request with named arguments transformed to positional arguments
    return out;
}

static bool ExecuteCommands(const std::vector<const CRPCCommand*>& commands, const JSONRPCRequest& request, UniValue& result)
{
    for (const auto& command : commands) {
        if (ExecuteCommand(*command, request, result, &command == &commands.back())) {
            return true;
        }
    }
    return false;
}

UniValue CRPCTable::execute(const JSONRPCRequest &request) const
{
    // Return immediately if in warmup
    {
        LOCK(g_rpc_warmup_mutex);
        if (fRPCInWarmup)
            throw JSONRPCError(RPC_IN_WARMUP, rpcWarmupStatus);
    }

    // Find method
    auto it = mapCommands.find(request.strMethod);
    if (it != mapCommands.end()) {
        UniValue result;
        if (ExecuteCommands(it->second, request, result)) {
            return result;
        }
    }
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");
}

static bool ExecuteCommand(const CRPCCommand& command, const JSONRPCRequest& request, UniValue& result, bool last_handler)
{
    try {
        RPCCommandExecution execution(request.strMethod);
        // Execute, convert arguments to array if necessary
        if (request.params.isObject()) {
            return command.actor(transformNamedArguments(request, command.argNames), result, last_handler);
        } else {
            return command.actor(request, result, last_handler);
        }
    } catch (const UniValue::type_error& e) {
        throw JSONRPCError(RPC_TYPE_ERROR, e.what());
    } catch (const std::exception& e) {
        throw JSONRPCError(RPC_MISC_ERROR, e.what());
    }
}

std::vector<std::string> CRPCTable::listCommands() const
{
    std::vector<std::string> commandList;
    commandList.reserve(mapCommands.size());
    for (const auto& i : mapCommands) commandList.emplace_back(i.first);
    return commandList;
}

UniValue CRPCTable::dumpArgMap(const JSONRPCRequest& args_request) const
{
    JSONRPCRequest request = args_request;
    request.mode = JSONRPCRequest::GET_ARGS;

    UniValue ret{UniValue::VARR};
    for (const auto& cmd : mapCommands) {
        UniValue result;
        if (ExecuteCommands(cmd.second, request, result)) {
            for (const auto& values : result.getValues()) {
                ret.push_back(values);
            }
        }
    }
    return ret;
}

void RPCSetTimerInterfaceIfUnset(RPCTimerInterface *iface)
{
    if (!timerInterface)
        timerInterface = iface;
}

void RPCSetTimerInterface(RPCTimerInterface *iface)
{
    timerInterface = iface;
}

void RPCUnsetTimerInterface(RPCTimerInterface *iface)
{
    if (timerInterface == iface)
        timerInterface = nullptr;
}

void RPCRunLater(const std::string& name, std::function<void()> func, int64_t nSeconds)
{
    if (!timerInterface)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No timer handler registered for RPC");
    LOCK(g_deadline_timers_mutex);
    deadlineTimers.erase(name);
    LogPrint(BCLog::RPC, "queue run of timer %s in %i seconds (using %s)\n", name, nSeconds, timerInterface->Name());
    deadlineTimers.emplace(name, std::unique_ptr<RPCTimerBase>(timerInterface->NewTimer(func, nSeconds*1000)));
}

int RPCSerializationFlags()
{
    int flag = 0;
    if (gArgs.GetIntArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) == 0)
        flag |= SERIALIZE_TRANSACTION_NO_WITNESS;
    return flag;
}

CRPCTable tableRPC;
