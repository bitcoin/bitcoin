// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <rpc/server.h>

#include <common/args.h>
#include <common/system.h>
#include <logging.h>
#include <node/context.h>
#include <node/kernel_notifications.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <sync.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <span>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

using util::SplitString;

static GlobalMutex g_rpc_warmup_mutex;
static std::atomic<bool> g_rpc_running{false};
static bool fRPCInWarmup GUARDED_BY(g_rpc_warmup_mutex) = true;
static std::string rpcWarmupStatus GUARDED_BY(g_rpc_warmup_mutex) = "RPC server started";
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

std::string CRPCTable::help(std::string_view strCommand, const JSONRPCRequest& helpreq) const
{
    std::string strRet;
    std::string category;
    std::set<intptr_t> setDone;
    std::vector<std::pair<std::string, const CRPCCommand*> > vCommands;
    vCommands.reserve(mapCommands.size());

    for (const auto& entry : mapCommands)
        vCommands.emplace_back(entry.second.front()->category + entry.first, entry.second.front());
    std::ranges::sort(vCommands);

    JSONRPCRequest jreq = helpreq;
    jreq.mode = JSONRPCRequest::GET_HELP;
    jreq.params = UniValue();

    for (const auto& [_, pcmd] : vCommands) {
        std::string strMethod = pcmd->name;
        if ((strCommand != "" || pcmd->category == "hidden") && strMethod != strCommand)
            continue;
        jreq.strMethod = strMethod;
        try
        {
            UniValue unused_result;
            if (setDone.insert(pcmd->unique_id).second)
                pcmd->actor(jreq, unused_result, /*last_handler=*/true);
        } catch (const HelpResult& e) {
            std::string strHelp{e.what()};
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
    return RPCHelpMan{
        "help",
        "List all commands, or get help for a specified command.\n",
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
    auto command{self.MaybeArg<std::string_view>("command")};
    if (command == "dump_all_command_conversions") {
        // Used for testing only, undocumented
        return tableRPC.dumpArgMap(jsonRequest);
    }

    return tableRPC.help(command.value_or(""), jsonRequest);
},
    };
}

static RPCHelpMan stop()
{
    static const std::string RESULT{CLIENT_NAME " stopping"};
    return RPCHelpMan{
        "stop",
    // Also accept the hidden 'wait' integer argument (milliseconds)
    // For instance, 'stop 1000' makes the call wait 1 second before returning
    // to the client (intended for testing)
        "Request a graceful shutdown of " CLIENT_NAME ".",
                {
                    {"wait", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "how long to wait in ms", RPCArgOptions{.hidden=true}},
                },
                RPCResult{RPCResult::Type::STR, "", "A string with the content '" + RESULT + "'"},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& jsonRequest) -> UniValue
{
    // Event loop will exit after current HTTP requests have been handled, so
    // this reply will get back to the client.
    CHECK_NONFATAL((CHECK_NONFATAL(EnsureAnyNodeContext(jsonRequest.context).shutdown_request))());
    if (jsonRequest.params[0].isNum()) {
        UninterruptibleSleep(std::chrono::milliseconds{jsonRequest.params[0].getInt<int>()});
    }
    return RESULT;
},
    };
}

static RPCHelpMan uptime()
{
    return RPCHelpMan{
        "uptime",
        "Returns the total uptime of the server.\n",
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
    return TicksSeconds(GetUptime());
}
    };
}

static RPCHelpMan getrpcinfo()
{
    return RPCHelpMan{
        "getrpcinfo",
        "Returns details of the RPC server.\n",
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
        entry.pushKV("duration", Ticks<std::chrono::microseconds>(SteadyClock::now() - info.start));
        active_commands.push_back(std::move(entry));
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("active_commands", std::move(active_commands));

    const std::string path = LogInstance().m_file_path.utf8string();
    UniValue log_path(UniValue::VSTR, path);
    result.pushKV("logpath", std::move(log_path));

    return result;
}
    };
}

namespace {
UniValue OpenRPCArgSchema(const RPCArg& arg);
UniValue OpenRPCResultSchema(const RPCResult& result);

UniValue MakeObject(std::initializer_list<std::pair<std::string, UniValue>> entries)
{
    UniValue obj{UniValue::VOBJ};
    for (const auto& [key, value] : entries) {
        obj.pushKV(key, value);
    }
    return obj;
}

void PushUniqueSchema(UniValue& schemas, std::unordered_set<std::string>& seen, UniValue schema)
{
    const std::string serialized{schema.write()};
    if (seen.insert(serialized).second) schemas.push_back(std::move(schema));
}

// NOLINTNEXTLINE(misc-no-recursion)
UniValue DedupArrayItemsSchema(std::span<const RPCArg> inner)
{
    if (inner.empty()) return UniValue{UniValue::VOBJ};
    if (inner.size() == 1) return OpenRPCArgSchema(inner.front());

    UniValue one_of{UniValue::VARR};
    std::unordered_set<std::string> seen;
    for (const auto& item : inner) {
        PushUniqueSchema(one_of, seen, OpenRPCArgSchema(item));
    }

    if (one_of.size() == 1) return one_of[0];

    UniValue items{UniValue::VOBJ};
    items.pushKV("oneOf", std::move(one_of));
    return items;
}

// NOLINTNEXTLINE(misc-no-recursion)
UniValue DedupArrayItemsSchema(std::span<const RPCResult> inner)
{
    if (inner.empty()) return UniValue{UniValue::VOBJ};
    if (inner.size() == 1) return OpenRPCResultSchema(inner.front());

    UniValue one_of{UniValue::VARR};
    std::unordered_set<std::string> seen;
    for (const auto& item : inner) {
        PushUniqueSchema(one_of, seen, OpenRPCResultSchema(item));
    }

    if (one_of.size() == 1) return one_of[0];

    UniValue items{UniValue::VOBJ};
    items.pushKV("oneOf", std::move(one_of));
    return items;
}

// NOLINTNEXTLINE(misc-no-recursion)
UniValue OpenRPCArgSchema(const RPCArg& arg)
{
    switch (arg.m_type) {
    case RPCArg::Type::STR:
        return MakeObject({{"type", "string"}});
    case RPCArg::Type::STR_HEX:
        return MakeObject({{"type", "string"}, {"pattern", "^[0-9a-fA-F]+$"}});
    case RPCArg::Type::NUM:
        return MakeObject({{"type", "number"}});
    case RPCArg::Type::BOOL:
        return MakeObject({{"type", "boolean"}});
    case RPCArg::Type::AMOUNT: {
        UniValue one_of{UniValue::VARR};
        one_of.push_back(MakeObject({{"type", "number"}}));
        one_of.push_back(MakeObject({{"type", "string"}}));
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("oneOf", std::move(one_of));
        return schema;
    }
    case RPCArg::Type::RANGE: {
        UniValue prefix_items{UniValue::VARR};
        prefix_items.push_back(MakeObject({{"type", "number"}}));
        prefix_items.push_back(MakeObject({{"type", "number"}}));
        UniValue range_schema{UniValue::VOBJ};
        range_schema.pushKV("type", "array");
        range_schema.pushKV("prefixItems", std::move(prefix_items));
        range_schema.pushKV("minItems", 2);
        range_schema.pushKV("maxItems", 2);
        UniValue one_of{UniValue::VARR};
        one_of.push_back(MakeObject({{"type", "number"}}));
        one_of.push_back(std::move(range_schema));
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("oneOf", std::move(one_of));
        return schema;
    }
    case RPCArg::Type::ARR: {
        UniValue items{DedupArrayItemsSchema(arg.m_inner)};
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "array");
        schema.pushKV("items", std::move(items));
        return schema;
    }
    case RPCArg::Type::OBJ:
    case RPCArg::Type::OBJ_NAMED_PARAMS: {
        UniValue properties{UniValue::VOBJ};
        UniValue required{UniValue::VARR};
        for (const auto& inner : arg.m_inner) {
            if (inner.m_opts.hidden) continue;
            UniValue prop{OpenRPCArgSchema(inner)};
            if (!inner.m_description.empty()) prop.pushKV("description", inner.m_description);
            if (inner.m_opts.placeholder) prop.pushKV("x-bitcoin-placeholder", true);
            if (inner.m_opts.also_positional) prop.pushKV("x-bitcoin-also-positional", true);
            properties.pushKV(inner.GetFirstName(), std::move(prop));
            if (!inner.IsOptional()) required.push_back(inner.GetFirstName());
        }
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "object");
        schema.pushKV("properties", std::move(properties));
        schema.pushKV("additionalProperties", false);
        if (!required.empty()) schema.pushKV("required", std::move(required));
        return schema;
    }
    case RPCArg::Type::OBJ_USER_KEYS: {
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "object");
        if (!arg.m_inner.empty()) {
            schema.pushKV("additionalProperties", OpenRPCArgSchema(arg.m_inner[0]));
        } else {
            schema.pushKV("additionalProperties", true);
        }
        return schema;
    }
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

// NOLINTNEXTLINE(misc-no-recursion)
UniValue OpenRPCResultSchema(const RPCResult& result)
{
    switch (result.m_type) {
    case RPCResult::Type::STR:
    case RPCResult::Type::STR_AMOUNT:
        return MakeObject({{"type", "string"}});
    case RPCResult::Type::STR_HEX:
        return MakeObject({{"type", "string"}, {"pattern", "^[0-9a-fA-F]+$"}});
    case RPCResult::Type::NUM:
        return MakeObject({{"type", "number"}});
    case RPCResult::Type::NUM_TIME: {
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "number");
        schema.pushKV("x-bitcoin-unit", "unix-time");
        return schema;
    }
    case RPCResult::Type::BOOL:
        return MakeObject({{"type", "boolean"}});
    case RPCResult::Type::NONE:
        return MakeObject({{"type", "null"}});
    case RPCResult::Type::ARR: {
        UniValue items{DedupArrayItemsSchema(result.m_inner)};
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "array");
        schema.pushKV("items", std::move(items));
        return schema;
    }
    case RPCResult::Type::ARR_FIXED: {
        UniValue prefix_items{UniValue::VARR};
        for (const auto& inner : result.m_inner) {
            prefix_items.push_back(OpenRPCResultSchema(inner));
        }
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "array");
        schema.pushKV("prefixItems", std::move(prefix_items));
        schema.pushKV("minItems", uint64_t(result.m_inner.size()));
        schema.pushKV("maxItems", uint64_t(result.m_inner.size()));
        return schema;
    }
    case RPCResult::Type::OBJ: {
        UniValue properties{UniValue::VOBJ};
        UniValue required{UniValue::VARR};
        bool has_elision{false};
        for (const auto& inner : result.m_inner) {
            if (inner.m_type == RPCResult::Type::ELISION) {
                has_elision = true;
                continue;
            }
            if (inner.m_key_name.empty()) continue;
            UniValue prop{OpenRPCResultSchema(inner)};
            if (!inner.m_description.empty()) prop.pushKV("description", inner.m_description);
            properties.pushKV(inner.m_key_name, std::move(prop));
            if (!inner.m_optional) required.push_back(inner.m_key_name);
        }
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "object");
        schema.pushKV("properties", std::move(properties));
        schema.pushKV("additionalProperties", has_elision);
        if (!required.empty()) schema.pushKV("required", std::move(required));
        return schema;
    }
    case RPCResult::Type::OBJ_DYN: {
        UniValue schema{UniValue::VOBJ};
        schema.pushKV("type", "object");
        if (!result.m_inner.empty()) {
            schema.pushKV("additionalProperties", OpenRPCResultSchema(result.m_inner[0]));
        } else {
            schema.pushKV("additionalProperties", UniValue{UniValue::VOBJ});
        }
        return schema;
    }
    case RPCResult::Type::ANY:
    case RPCResult::Type::ELISION:
        return UniValue{UniValue::VOBJ};
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}
} // namespace

static RPCHelpMan getopenrpcinfo()
{
    return RPCHelpMan{
        "getopenrpcinfo",
        "Returns an OpenRPC document for currently available RPC commands.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "openrpc", "OpenRPC specification version."},
                {RPCResult::Type::OBJ, "info", "Metadata about this JSON-RPC interface.",
                    {
                        {RPCResult::Type::STR, "title", "API title."},
                        {RPCResult::Type::STR, "version", "Bitcoin Core version string."},
                        {RPCResult::Type::STR, "description", "API description."},
                    }},
                {RPCResult::Type::ARR, "methods", "Documented RPC methods.",
                    {{RPCResult::Type::OBJ, "", "An RPC method description object.",
                        {
                            {RPCResult::Type::STR, "name", "Method name."},
                            {RPCResult::Type::STR, "description", "Method description."},
                            {RPCResult::Type::ARR, "params", "Method parameters.",
                                {{RPCResult::Type::OBJ, "", "A parameter.",
                                    {
                                        {RPCResult::Type::STR, "name", "Parameter name."},
                                        {RPCResult::Type::BOOL, "required", "Whether the parameter is required."},
                                        {RPCResult::Type::ANY, "schema", "JSON Schema for the parameter."},
                                        {RPCResult::Type::STR, "description", /*optional=*/true, "Parameter description."},
                                        {RPCResult::Type::ARR, "x-bitcoin-aliases", /*optional=*/true, "Alternative parameter names.",
                                            {{RPCResult::Type::STR, "", "An alias."}}},
                                        {RPCResult::Type::BOOL, "x-bitcoin-placeholder", /*optional=*/true, "Whether the parameter is retained only for compatibility."},
                                        {RPCResult::Type::BOOL, "x-bitcoin-also-positional", /*optional=*/true, "Whether the parameter can also be passed positionally."},
                                    }}}},
                            {RPCResult::Type::OBJ, "result", "Method result.",
                                {
                                    {RPCResult::Type::STR, "name", "Result name."},
                                    {RPCResult::Type::ANY, "schema", "JSON Schema for the result."},
                                }},
                            {RPCResult::Type::STR, "x-bitcoin-category", "RPC category."},
                        }}}},
            },
            {.skip_type_check = true}},
        RPCExamples{
            HelpExampleCli("getopenrpcinfo", "")
            + HelpExampleRpc("getopenrpcinfo", "")
        },
        [](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return tableRPC.buildOpenRPCDoc();
},
    };
}

static const CRPCCommand vRPCCommands[]{
    /* Overall control/query calls */
    {"control", &getopenrpcinfo},
    {"control", &getrpcinfo},
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
            if (it->second.empty()) {
                mapCommands.erase(it);
            }
            return true;
        }
    }
    return false;
}

void StartRPC()
{
    LogDebug(BCLog::RPC, "Starting RPC\n");
    g_rpc_running = true;
}

void InterruptRPC()
{
    static std::once_flag g_rpc_interrupt_flag;
    // This function could be called twice if the GUI has been started with -server=1.
    std::call_once(g_rpc_interrupt_flag, []() {
        LogDebug(BCLog::RPC, "Interrupting RPC\n");
        // Interrupt e.g. running longpolls
        g_rpc_running = false;
    });
}

void StopRPC()
{
    static std::once_flag g_rpc_stop_flag;
    // This function could be called twice if the GUI has been started with -server=1.
    assert(!g_rpc_running);
    std::call_once(g_rpc_stop_flag, [&]() {
        LogDebug(BCLog::RPC, "Stopping RPC\n");
        DeleteAuthCookie();
        LogDebug(BCLog::RPC, "RPC stopped.\n");
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

void SetRPCWarmupStarting()
{
    LOCK(g_rpc_warmup_mutex);
    fRPCInWarmup = true;
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

UniValue JSONRPCExec(const JSONRPCRequest& jreq, bool catch_errors)
{
    UniValue result;
    if (catch_errors) {
        try {
            result = tableRPC.execute(jreq);
        } catch (UniValue& e) {
            return JSONRPCReplyObj(NullUniValue, std::move(e), jreq.id, jreq.m_json_version);
        } catch (const std::exception& e) {
            return JSONRPCReplyObj(NullUniValue, JSONRPCError(RPC_MISC_ERROR, e.what()), jreq.id, jreq.m_json_version);
        }
    } else {
        result = tableRPC.execute(jreq);
    }

    return JSONRPCReplyObj(std::move(result), NullUniValue, jreq.id, jreq.m_json_version);
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
    // inserted at the unspecified parameter positions, and the "hole" variable
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

UniValue CRPCTable::buildOpenRPCDoc() const
{
    std::vector<std::string> method_names;
    for (const auto& [name, cmds] : mapCommands) {
        if (cmds.empty()) continue;
        const CRPCCommand* cmd{cmds.front()};
        if (cmd->category == "hidden" || !cmd->metadata_fn) continue;
        method_names.push_back(name);
    }
    std::sort(method_names.begin(), method_names.end());

    UniValue methods{UniValue::VARR};
    for (const auto& method_name : method_names) {
        const CRPCCommand* cmd{mapCommands.at(method_name).front()};
        RPCHelpMan helpman{cmd->metadata_fn()};

        UniValue params{UniValue::VARR};
        for (const auto& arg : helpman.GetArgs()) {
            if (arg.m_opts.hidden) continue;
            UniValue param{UniValue::VOBJ};
            param.pushKV("name", arg.GetFirstName());
            param.pushKV("required", !arg.IsOptional());
            param.pushKV("schema", OpenRPCArgSchema(arg));

            std::vector<std::string> names{SplitString(arg.m_names, '|')};
            if (names.size() > 1) {
                UniValue aliases{UniValue::VARR};
                for (size_t i{1}; i < names.size(); ++i) aliases.push_back(names[i]);
                param.pushKV("x-bitcoin-aliases", std::move(aliases));
            }
            if (arg.m_opts.placeholder) param.pushKV("x-bitcoin-placeholder", true);
            if (arg.m_opts.also_positional) param.pushKV("x-bitcoin-also-positional", true);
            if (!arg.m_description.empty()) param.pushKV("description", arg.m_description);
            params.push_back(std::move(param));
        }

        UniValue result_schema{UniValue::VOBJ};
        const auto& results{helpman.GetResults().m_results};
        if (results.size() == 1 && results[0].m_type != RPCResult::Type::ANY) {
            result_schema = OpenRPCResultSchema(results[0]);
        } else if (results.size() > 1) {
            UniValue one_of{UniValue::VARR};
            for (const auto& r : results) {
                if (r.m_type == RPCResult::Type::ANY) continue;
                UniValue schema{OpenRPCResultSchema(r)};
                if (!r.m_cond.empty()) schema.pushKV("description", r.m_cond);
                one_of.push_back(std::move(schema));
            }
            if (one_of.size() == 1) {
                result_schema = one_of[0];
            } else if (one_of.size() > 1) {
                result_schema.pushKV("oneOf", std::move(one_of));
            }
        }

        UniValue method{UniValue::VOBJ};
        method.pushKV("name", method_name);
        method.pushKV("description", util::TrimString(helpman.GetDescription()));
        method.pushKV("params", std::move(params));
        UniValue result{UniValue::VOBJ};
        result.pushKV("name", "result");
        result.pushKV("schema", std::move(result_schema));
        method.pushKV("result", std::move(result));
        method.pushKV("x-bitcoin-category", cmd->category);
        methods.push_back(std::move(method));
    }

    std::string version{"v" CLIENT_VERSION_STRING};
    if (!CLIENT_VERSION_IS_RELEASE) version += "-dev";

    UniValue info{UniValue::VOBJ};
    info.pushKV("title", "Bitcoin Core JSON-RPC");
    info.pushKV("version", version);
    info.pushKV("description", "Autogenerated from Bitcoin Core RPC metadata.");

    UniValue doc{UniValue::VOBJ};
    doc.pushKV("openrpc", "1.3.2");
    doc.pushKV("info", std::move(info));
    doc.pushKV("methods", std::move(methods));
    return doc;
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

CRPCTable tableRPC;
