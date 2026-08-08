// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <index/txospenderindex.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <kernel/cs_main.h>
#include <logging.h>
#include <node/context.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <scheduler.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/any.h>
#include <util/check.h>
#include <util/time.h>

#include <cstdint>
#include <limits>
#ifdef HAVE_MALLOC_INFO
#include <malloc.h>
#endif
#include <string_view>

using node::NodeContext;

static RPCMethod setmocktime()
{
    return RPCMethod{
        "setmocktime",
        "Set the local time to given timestamp (-regtest only)\n",
        {
            {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, UNIX_EPOCH_TIME + "\n"
             "Pass 0 to go back to using the system time."},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    if (!Params().IsMockableChain()) {
        throw std::runtime_error("setmocktime is for regression testing (-regtest mode) only");
    }

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsCurrentForFeeEstimation() and IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all call sites of GetTime() are accessing this safely.
    LOCK(cs_main);

    const int64_t time{request.params[0].getInt<int64_t>()};
    // block timestamps are uint32_t, so mocking time beyond that is meaningless for anything
    // consensus-related and can cause integer overflow/truncation issues in time arithmetic.
    constexpr int64_t max_time{std::numeric_limits<uint32_t>::max()};
    if (time < 0 || time > max_time) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Mocktime must be in the range [0, %s], not %s.", max_time, time));
    }

    SetMockTime(time);
    const NodeContext& node_context{EnsureAnyNodeContext(request.context)};
    for (const auto& chain_client : node_context.chain_clients) {
        chain_client->setMockTime(time);
    }

    return UniValue::VNULL;
},
    };
}

static RPCMethod mockscheduler()
{
    return RPCMethod{
        "mockscheduler",
        "Bump the scheduler into the future (-regtest only)\n",
        {
            {"delta_time", RPCArg::Type::NUM, RPCArg::Optional::NO, "Number of seconds to forward the scheduler into the future." },
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    if (!Params().IsMockableChain()) {
        throw std::runtime_error("mockscheduler is for regression testing (-regtest mode) only");
    }

    int64_t delta_seconds = request.params[0].getInt<int64_t>();
    if (delta_seconds <= 0 || delta_seconds > 3600) {
        throw std::runtime_error("delta_time must be between 1 and 3600 seconds (1 hr)");
    }

    const NodeContext& node_context{EnsureAnyNodeContext(request.context)};
    CHECK_NONFATAL(node_context.scheduler)->MockForward(std::chrono::seconds{delta_seconds});
    CHECK_NONFATAL(node_context.validation_signals)->SyncWithValidationInterfaceQueue();
    for (const auto& chain_client : node_context.chain_clients) {
        chain_client->schedulerMockForward(std::chrono::seconds(delta_seconds));
    }

    return UniValue::VNULL;
},
    };
}

static UniValue RPCLockedMemoryInfo()
{
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", stats.used);
    obj.pushKV("free", stats.free);
    obj.pushKV("total", stats.total);
    obj.pushKV("locked", stats.locked);
    obj.pushKV("chunks_used", stats.chunks_used);
    obj.pushKV("chunks_free", stats.chunks_free);
    return obj;
}

#ifdef HAVE_MALLOC_INFO
static std::string RPCMallocInfo()
{
    char *ptr = nullptr;
    size_t size = 0;
    FILE *f = open_memstream(&ptr, &size);
    if (f) {
        malloc_info(0, f);
        fclose(f);
        if (ptr) {
            std::string rv(ptr, size);
            free(ptr);
            return rv;
        }
    }
    return "";
}
#endif

static RPCMethod getmemoryinfo()
{
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    return RPCMethod{"getmemoryinfo",
                "Returns an object containing information about memory usage.\n",
                {
                    {"mode", RPCArg::Type::STR, RPCArg::Default{"stats"}, "determines what kind of information is returned.\n"
            "  - \"stats\" returns general statistics about memory usage in the daemon.\n"
            "  - \"mallocinfo\" returns an XML string describing low-level heap state (only available if compiled with glibc)."},
                },
                {
                    RPCResult{"mode \"stats\"",
                        RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::OBJ, "locked", "Information about locked memory manager",
                            {
                                {RPCResult::Type::NUM, "used", "Number of bytes used"},
                                {RPCResult::Type::NUM, "free", "Number of bytes available in current arenas"},
                                {RPCResult::Type::NUM, "total", "Total number of bytes managed"},
                                {RPCResult::Type::NUM, "locked", "Amount of bytes that succeeded locking. If this number is smaller than total, locking pages failed at some point and key data could be swapped to disk."},
                                {RPCResult::Type::NUM, "chunks_used", "Number allocated chunks"},
                                {RPCResult::Type::NUM, "chunks_free", "Number unused chunks"},
                            }},
                        }
                    },
                    RPCResult{"mode \"mallocinfo\"",
                        RPCResult::Type::STR, "", "\"<malloc version=\"1\">...\""
                    },
                },
                RPCExamples{
                    HelpExampleCli("getmemoryinfo", "")
            + HelpExampleRpc("getmemoryinfo", "")
                },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    auto mode{self.Arg<std::string_view>("mode")};
    if (mode == "stats") {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("locked", RPCLockedMemoryInfo());
        return obj;
    } else if (mode == "mallocinfo") {
#ifdef HAVE_MALLOC_INFO
        return RPCMallocInfo();
#else
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mallocinfo mode not available");
#endif
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, tfm::format("unknown mode %s", mode));
    }
},
    };
}

// Apply a list of (category flag, level) changes.
static void UpdateLogCategories(std::vector<std::pair<BCLog::LogFlags, BCLog::Level>> changes)
{
    for (const auto& [category, level] : changes) {
        if (category == BCLog::ALL) {
            LogInstance().SetLogLevel(level);
            LogInstance().SetCategoryLogLevel({});
        } else {
            LogInstance().AddCategoryLogLevel(category, level);
        }
        if (level >= util::log::Level::Info) {
            LogInstance().DisableCategory(category);
        } else {
            LogInstance().EnableCategory(category);
        }
    }
}

static RPCMethod loglevel()
{
    std::vector<RPCArg> category_args;
    for (const auto& cat : LogInstance().LogCategoriesList()) {
        category_args.emplace_back(cat.category, RPCArg::Type::STR, RPCArg::Optional::OMITTED,
            "log level for the \"" + cat.category + "\" category");
    }
    return RPCMethod{"loglevel",
            "Gets and sets per-category log levels.\n"
            "When called without arguments, returns all log categories with their current log level.\n"
            "When called with arguments, sets the log level for specified categories,\n"
            "then returns the updated state of all categories.\n"
            "The valid log levels are: " + LogInstance().LogLevelsString() + "\n"
            ,
                {
                    {"all", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Log level to set for all categories."},
                    {"categories", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "Per-category log levels.", std::move(category_args)},
                },
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "keys are the logging categories, values are their current log levels",
                    {
                        {RPCResult::Type::STR, "category", "current log level"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("loglevel", "")
                  + HelpExampleCli("loglevel", "debug")
                  + HelpExampleCli("-named loglevel", "info net=debug")
                  + HelpExampleRpc("loglevel", "\"debug\"")
                },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    std::vector<std::pair<BCLog::LogFlags, BCLog::Level>> changes;

    // Optional named and positional "all" param.
    if (auto level_str{self.MaybeArg<std::string_view>("all")}) {
        const auto level = BCLog::Logger::GetLogLevel(*level_str);
        if (!level || *level > BCLog::Level::Info) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, tfm::format("unknown log level \"%s\". Valid values: %s", *level_str, LogInstance().LogLevelsString()));
        }
        changes.emplace_back(BCLog::ALL, *level);
    }

    // Named "categories" params: applied in the order they appear in the request.
    // Category names are validated by OBJ_NAMED_PARAMS, so GetLogCategory always succeeds here.
    if (auto cats{self.MaybeArg<UniValue>("categories")}) {
        for (const std::string& cat : cats->getKeys()) {
            const std::string level_str = (*cats)[cat].get_str();
            const auto level = BCLog::Logger::GetLogLevel(level_str);
            if (!level || *level > BCLog::Level::Info) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown log level \"" + level_str + "\". Valid values: " + LogInstance().LogLevelsString());
            }
            changes.emplace_back(*BCLog::Logger::GetLogCategory(cat), *level);
        }
    }

    if (!changes.empty()) UpdateLogCategories(std::move(changes));

    UniValue result(UniValue::VOBJ);
    for (const auto& logCat : LogInstance().LogCategoriesList()) {
        result.pushKV(logCat.category, BCLog::Logger::LogLevelToStr(logCat.level));
    }

    return result;
},
    };
}

static RPCMethod logging()
{
    return RPCMethod{"logging",
            "Gets and sets the logging configuration.\n"
            "When called without an argument, returns the list of categories with status that are currently being debug logged or not.\n"
            "When called with arguments, adds or removes categories from debug logging and return the lists above.\n"
            "The arguments are evaluated in order \"include\", \"exclude\".\n"
            "If an item is both included and excluded, it will thus end up being excluded.\n"
            "The valid logging categories are: " + LogInstance().LogCategoriesString() + "\n"
            "In addition, the following are available as category names with special meanings:\n"
            "  - \"all\",  \"1\" : represent all logging categories.\n"
            "See also: the \"loglevel\" RPC, which provides a superset of functionality, allowing trace logs to be enabled in addition to debug logs.\n"
            ,
                {
                    {"include", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The categories to add to debug logging",
                        {
                            {"include_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                    {"exclude", RPCArg::Type::ARR, RPCArg::Optional::OMITTED, "The categories to remove from debug logging",
                        {
                            {"exclude_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                },
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "keys are the logging categories, and values indicates its status",
                    {
                        {RPCResult::Type::BOOL, "category", "if being debug logged or not. false:inactive, true:active"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("logging", "\"[\\\"all\\\"]\" \"[\\\"http\\\"]\"")
            + HelpExampleRpc("logging", "[\"all\"], [\"leveldb\"]")
                },
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    std::vector<std::pair<BCLog::LogFlags, BCLog::Level>> changes;
    auto parse_categories = [&](const UniValue& cats, BCLog::Level level) {
        for (const UniValue& cat_val : cats.get_array().getValues()) {
            const std::string& cat = cat_val.get_str();
            const auto flag{BCLog::Logger::GetLogCategory(cat)};
            if (!flag) throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown logging category " + cat);
            changes.emplace_back(*flag, level);
        }
    };
    if (request.params[0].isArray()) parse_categories(request.params[0], BCLog::Level::Debug);
    if (request.params[1].isArray()) parse_categories(request.params[1], BCLog::Level::Info);
    UpdateLogCategories(std::move(changes));

    UniValue result(UniValue::VOBJ);
    for (const auto& logCat : LogInstance().LogCategoriesList()) {
        result.pushKV(logCat.category, logCat.level < BCLog::Level::Info);
    }

    return result;
},
    };
}

static RPCMethod echo(const std::string& name)
{
    return RPCMethod{
        name,
        "Simply echo back the input arguments. This command is for testing.\n"
                "\nIt will return an internal bug report when arg9='trigger_internal_bug' is passed.\n"
                "\nThe difference between echo and echojson is that echojson has argument conversion enabled in the client-side table in "
                "bitcoin-cli and the GUI. There is no server-side difference.",
        {
            {"arg0", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg1", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg2", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg3", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg4", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg5", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg6", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg7", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg8", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
            {"arg9", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "", RPCArgOptions{.skip_type_check = true}},
        },
                RPCResult{RPCResult::Type::ANY, "", "Returns whatever was passed in"},
                RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    if (request.params[9].isStr()) {
        CHECK_NONFATAL(request.params[9].get_str() != "trigger_internal_bug");
    }

    return request.params;
},
    };
}

static RPCMethod echo() { return echo("echo"); }
static RPCMethod echojson() { return echo("echojson"); }

static RPCMethod echoipc()
{
    return RPCMethod{
        "echoipc",
        "Echo back the input argument, passing it through a spawned process in a multiprocess build.\n"
        "This command is for testing.\n",
        {{"arg", RPCArg::Type::STR, RPCArg::Optional::NO, "The string to echo",}},
        RPCResult{RPCResult::Type::STR, "echo", "The echoed string."},
        RPCExamples{HelpExampleCli("echo", "\"Hello world\"") +
                    HelpExampleRpc("echo", "\"Hello world\"")},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue {
            interfaces::Init& local_init = *EnsureAnyNodeContext(request.context).init;
            std::unique_ptr<interfaces::Echo> echo;
            if (interfaces::Ipc* ipc = local_init.ipc()) {
                // Spawn a new bitcoin-node process and call makeEcho to get a
                // client pointer to a interfaces::Echo instance running in
                // that process. This is just for testing. A slightly more
                // realistic test spawning a different executable instead of
                // the same executable would add a new bitcoin-echo executable,
                // and spawn bitcoin-echo below instead of bitcoin-node. But
                // using bitcoin-node avoids the need to build and install a
                // new executable just for this one test.
                auto init = ipc->spawnProcess("bitcoin-node");
                echo = init->makeEcho();
                ipc->addCleanup(*echo, [init = init.release()] { delete init; });
            } else {
                // IPC support is not available because this is a bitcoind
                // process not a bitcoind-node process, so just create a local
                // interfaces::Echo object and return it so the `echoipc` RPC
                // method will work, and the python test calling `echoipc`
                // can expect the same result.
                echo = local_init.makeEcho();
            }
            return echo->echo(request.params[0].get_str());
        },
    };
}

static UniValue SummaryToJSON(const IndexSummary&& summary, std::string index_name)
{
    UniValue ret_summary(UniValue::VOBJ);
    if (!index_name.empty() && index_name != summary.name) return ret_summary;

    UniValue entry(UniValue::VOBJ);
    entry.pushKV("synced", summary.synced);
    entry.pushKV("best_block_height", summary.best_block_height);
    ret_summary.pushKV(summary.name, std::move(entry));
    return ret_summary;
}

static RPCMethod getindexinfo()
{
    return RPCMethod{
        "getindexinfo",
        "Returns the status of one or all available indices currently running in the node.\n",
                {
                    {"index_name", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Filter results for an index with a specific name."},
                },
                RPCResult{
                    RPCResult::Type::OBJ_DYN, "", "", {
                        {
                            RPCResult::Type::OBJ, "name", "The name of the index",
                            {
                                {RPCResult::Type::BOOL, "synced", "Whether the index is synced or not"},
                                {RPCResult::Type::NUM, "best_block_height", "The block height to which the index is synced"},
                            }
                        },
                    },
                },
                RPCExamples{
                    HelpExampleCli("getindexinfo", "")
                  + HelpExampleRpc("getindexinfo", "")
                  + HelpExampleCli("getindexinfo", "txindex")
                  + HelpExampleRpc("getindexinfo", "txindex")
                },
                [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);
    const std::string index_name{self.MaybeArg<std::string_view>("index_name").value_or("")};

    if (g_txindex) {
        result.pushKVs(SummaryToJSON(g_txindex->GetSummary(), index_name));
    }

    if (g_coin_stats_index) {
        result.pushKVs(SummaryToJSON(g_coin_stats_index->GetSummary(), index_name));
    }

    if (g_txospenderindex) {
        result.pushKVs(SummaryToJSON(g_txospenderindex->GetSummary(), index_name));
    }

    ForEachBlockFilterIndex([&result, &index_name](const BlockFilterIndex& index) {
        result.pushKVs(SummaryToJSON(index.GetSummary(), index_name));
    });

    return result;
},
    };
}

void RegisterNodeRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"control", &getmemoryinfo},
        {"control", &loglevel},
        {"control", &logging},
        {"util", &getindexinfo},
        {"hidden", &setmocktime},
        {"hidden", &mockscheduler},
        {"hidden", &echo},
        {"hidden", &echojson},
        {"hidden", &echoipc},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
