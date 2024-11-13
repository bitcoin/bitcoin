#include <rpc/schema.h>

#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/string.h>

using util::SplitString;

// Notes
// =====
//
// This file implements the `schema` RPC. See `Schema::Commands` for the entry
// point. This RPC is intended to facilite writing code generators which can
// generate Bitcoin Core RPC clients in other languages. It is as
// self-contained as possible in this file, to facilitate back-porting to older
// versions and rebasing onto newer versions.
//
// We should probably use something like Open RPC, but the Bitcoin Core RPC API
// is weird enough that this may be difficult.
//
// The returned JSON includes all avaialable information about the RPC, whether
// useful to external callers or not. There is certainly more detail than
// necessary, and some of it should probably be elided.
//
// The top level type is a map of strings to `vector<CRPCCommand>`. This is
// because commands can have aliases, at least according to the types. However,
// I haven't actually seen one, so we just assert that there are no aliases so
// we don't have to worry about it.

class Schema {
public:
    static UniValue Commands(const std::map<std::string, std::vector<const CRPCCommand*>>& commands) {
        UniValue value{UniValue::VOBJ};

        UniValue rpcs{UniValue::VOBJ};

        for (const auto& entry: commands) {
            assert(entry.second.size() == 1);

            const auto& command = entry.second[0];

            RPCHelpMan rpc = ((RpcMethodFnType)command->unique_id)();

            rpcs.pushKV(entry.first, Schema::Command(command->category, rpc, command->argNames));
        }

        value.pushKV("rpcs", rpcs);

        return value;
    }

private:
    static UniValue Argument(const RPCArg& argument) {
        UniValue value{UniValue::VOBJ};

        UniValue names{UniValue::VARR};
        for (auto const& name: SplitString(argument.m_names, '|')) {
            names.push_back(name);
        }
        value.pushKV("names", names);

        value.pushKV("description", argument.m_description);

        value.pushKV("oneline_description", argument.m_opts.oneline_description);

        value.pushKV("also_positional", argument.m_opts.also_positional);

        UniValue type_str{UniValue::VARR};
        for (auto const& str: argument.m_opts.type_str) {
            type_str.push_back(str);
        }
        value.pushKV("type_str", type_str);

        bool required = std::holds_alternative<RPCArg::Optional>(argument.m_fallback)
            && std::get<RPCArg::Optional>(argument.m_fallback) == RPCArg::Optional::NO;

        value.pushKV("required", required);

        if (std::holds_alternative<UniValue>(argument.m_fallback)) {
            value.pushKV("default", std::get<UniValue>(argument.m_fallback));
        }

        if (std::holds_alternative<std::string>(argument.m_fallback)) {
            value.pushKV("default_hint", std::get<std::string>(argument.m_fallback));
        }

        value.pushKV("hidden", argument.m_opts.hidden);

        value.pushKV("type", Schema::ArgumentType(argument.m_type));

        UniValue inner{UniValue::VARR};
        for (auto const& argument: argument.m_inner) {
            inner.push_back(Schema::Argument(argument));
        }
        if (!inner.empty()) {
            value.pushKV("inner", inner);
        }

        return value;
    }

    static std::string ArgumentType(const RPCArg::Type& type) {
        switch (type) {
            case RPCArg::Type::AMOUNT:
                return "amount";
            case RPCArg::Type::ARR:
                return "array";
            case RPCArg::Type::BOOL:
                return "boolean";
            case RPCArg::Type::NUM:
                return "number";
            case RPCArg::Type::OBJ:
                return "object";
            case RPCArg::Type::OBJ_NAMED_PARAMS:
                return "object";
            case RPCArg::Type::OBJ_USER_KEYS:
                return "object";
            case RPCArg::Type::RANGE:
                return "range";
            case RPCArg::Type::STR:
                return "string";
            case RPCArg::Type::STR_HEX:
                return "hex";
            default:
                NONFATAL_UNREACHABLE();
        }
    }

    static UniValue Command(
        const std::string& category,
        const RPCHelpMan& command,
        const std::vector<std::pair<std::string, bool>>& argNames
    ) {
        UniValue value{UniValue::VOBJ};

        value.pushKV("category", category);
        value.pushKV("description", command.m_description);
        value.pushKV("examples", command.m_examples.m_examples);
        value.pushKV("name", command.m_name);

        UniValue argument_names{UniValue::VARR};
        for (const auto& pair : argNames) {
            UniValue argument_name{UniValue::VARR};
            argument_names.push_back(pair.first);
        }
        value.pushKV("argument_names", argument_names);

        UniValue arguments{UniValue::VARR};
        for (const auto& argument : command.m_args) {
            arguments.push_back(Schema::Argument(argument));
        }
        value.pushKV("arguments", arguments);

        UniValue results{UniValue::VARR};
        for (const auto& result : command.m_results.m_results) {
            results.push_back(Schema::Result(result));
        }
        value.pushKV("results", results);

        return value;
    }

    static UniValue Result(const RPCResult& result) {
        UniValue value{UniValue::VOBJ};
        value.pushKV("type", Schema::ResultType(result.m_type));
        value.pushKV("optional", result.m_optional);
        value.pushKV("description", result.m_description);
        value.pushKV("skip_type_check", result.m_skip_type_check);
        value.pushKV("key_name", result.m_key_name);
        value.pushKV("condition", result.m_cond);

        UniValue inner{UniValue::VARR};
        for (auto const& result: result.m_inner) {
            inner.push_back(Schema::Result(result));
        }
        if (!inner.empty()) {
            value.pushKV("inner", inner);
        }

        return value;
    }

    static std::string ResultType(const RPCResult::Type& type) {
        switch (type) {
            case RPCResult::Type::OBJ:
                return "object";
            case RPCResult::Type::ARR:
                return "array";
            case RPCResult::Type::STR:
                return "string";
            case RPCResult::Type::NUM:
                return "number";
            case RPCResult::Type::BOOL:
                return "boolean";
            case RPCResult::Type::NONE:
                return "none";
            case RPCResult::Type::ANY:
                return "any";
            case RPCResult::Type::STR_AMOUNT:
                return "amount";
            case RPCResult::Type::STR_HEX:
                return "hex";
            case RPCResult::Type::OBJ_DYN:
                return "object";
            case RPCResult::Type::ARR_FIXED:
                return "object";
            case RPCResult::Type::NUM_TIME:
                return "timestamp";
            case RPCResult::Type::ELISION:
                return "elision";
            default:
                NONFATAL_UNREACHABLE();
        }
    }
};

UniValue CommandSchemas(const std::map<std::string, std::vector<const CRPCCommand*>>& commands) {
    return Schema::Commands(commands);
}
