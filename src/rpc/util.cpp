// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <outputtype.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <tuple>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

const std::string UNIX_EPOCH_TIME = "UNIX epoch time";
const std::string EXAMPLE_ADDRESS[2] = {"bc1q09vm5lfy0j5reeulh4x5752q25uqqvz34hufdl", "bc1q02ad21edsxd23d32dfgqqsz4vv4nmtfzuklhy3"};

void RPCTypeCheck(const UniValue& params,
                  const std::list<UniValueType>& typesExpected,
                  bool fAllowNull)
{
    unsigned int i = 0;
    for (const UniValueType& t : typesExpected) {
        if (params.size() <= i)
            break;

        const UniValue& v = params[i];
        if (!(fAllowNull && v.isNull())) {
            RPCTypeCheckArgument(v, t);
        }
        i++;
    }
}

void RPCTypeCheckArgument(const UniValue& value, const UniValueType& typeExpected)
{
    if (!typeExpected.typeAny && value.type() != typeExpected.type) {
        throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected type %s, got %s", uvTypeName(typeExpected.type), uvTypeName(value.type())));
    }
}

void RPCTypeCheckObj(const UniValue& o,
    const std::map<std::string, UniValueType>& typesExpected,
    bool fAllowNull,
    bool fStrict)
{
    for (const auto& t : typesExpected) {
        const UniValue& v = find_value(o, t.first);
        if (!fAllowNull && v.isNull())
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));

        if (!(t.second.typeAny || v.type() == t.second.type || (fAllowNull && v.isNull()))) {
            std::string err = strprintf("Expected type %s for %s, got %s",
                uvTypeName(t.second.type), t.first, uvTypeName(v.type()));
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }

    if (fStrict)
    {
        for (const std::string& k : o.getKeys())
        {
            if (typesExpected.count(k) == 0)
            {
                std::string err = strprintf("Unexpected key %s", k);
                throw JSONRPCError(RPC_TYPE_ERROR, err);
            }
        }
    }
}

CAmount AmountFromValue(const UniValue& value, int decimals)
{
    if (!value.isNum() && !value.isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), decimals, &amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!MoneyRange(amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
    return amount;
}

uint256 ParseHashV(const UniValue& v, std::string strName)
{
    std::string strHex(v.get_str());
    if (64 != strHex.length())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be of length %d (not %d, for '%s')", strName, 64, strHex.length(), strHex));
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return uint256S(strHex);
}
uint256 ParseHashO(const UniValue& o, std::string strKey)
{
    return ParseHashV(find_value(o, strKey), strKey);
}
std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName)
{
    std::string strHex;
    if (v.isStr())
        strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey)
{
    return ParseHexV(find_value(o, strKey), strKey);
}

namespace {

/**
 * Quote an argument for shell.
 *
 * @note This is intended for help, not for security-sensitive purposes.
 */
std::string ShellQuote(const std::string& s)
{
    std::string result;
    result.reserve(s.size() * 2);
    for (const char ch: s) {
        if (ch == '\'') {
            result += "'\''";
        } else {
            result += ch;
        }
    }
    return "'" + result + "'";
}

/**
 * Shell-quotes the argument if it needs quoting, else returns it literally, to save typing.
 *
 * @note This is intended for help, not for security-sensitive purposes.
 */
std::string ShellQuoteIfNeeded(const std::string& s)
{
    for (const char ch: s) {
        if (ch == ' ' || ch == '\'' || ch == '"') {
            return ShellQuote(s);
        }
    }

    return s;
}

}

std::string HelpExampleCli(const std::string& methodname, const std::string& args)
{
    return "> bitcoin-cli " + methodname + " " + args + "\n";
}

std::string HelpExampleCliNamed(const std::string& methodname, const RPCArgList& args)
{
    std::string result = "> bitcoin-cli -named " + methodname;
    for (const auto& argpair: args) {
        const auto& value = argpair.second.isStr()
                ? argpair.second.get_str()
                : argpair.second.write();
        result += " " + argpair.first + "=" + ShellQuoteIfNeeded(value);
    }
    result += "\n";
    return result;
}

std::string HelpExampleRpc(const std::string& methodname, const std::string& args)
{
    return "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\": \"curltest\", "
        "\"method\": \"" + methodname + "\", \"params\": [" + args + "]}' -H 'content-type: text/plain;' http://127.0.0.1:26145/\n";
}

std::string HelpExampleRpcNamed(const std::string& methodname, const RPCArgList& args)
{
    UniValue params(UniValue::VOBJ);
    for (const auto& param: args) {
        params.pushKV(param.first, param.second);
    }

    return "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\": \"curltest\", "
           "\"method\": \"" + methodname + "\", \"params\": " + params.write() + "}' -H 'content-type: text/plain;' http://127.0.0.1:26145/\n";
}

// Converts a hex string to a public key if possible
CPubKey HexToPubKey(const std::string& hex_in)
{
    if (!IsHex(hex_in)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid public key: " + hex_in);
    }
    CPubKey vchPubKey(ParseHex(hex_in));
    if (!vchPubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid public key: " + hex_in);
    }
    return vchPubKey;
}

// Retrieves a public key for an address from the given FillableSigningProvider
CPubKey AddrToPubKey(const FillableSigningProvider& keystore, const std::string& addr_in)
{
    CTxDestination dest = DecodeDestination(addr_in);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address: " + addr_in);
    }
    CKeyID key = GetKeyForDestination(keystore, dest);
    if (key.IsNull()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("%s does not refer to a key", addr_in));
    }
    CPubKey vchPubKey;
    if (!keystore.GetPubKey(key, vchPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("no full public key for address %s", addr_in));
    }
    if (!vchPubKey.IsFullyValid()) {
       throw JSONRPCError(RPC_INTERNAL_ERROR, "Wallet contains an invalid public key");
    }
    return vchPubKey;
}

// Creates a multisig address from a given list of public keys, number of signatures required, and the address type
CTxDestination AddAndGetMultisigDestination(const int required, const std::vector<CPubKey>& pubkeys, OutputType type, FillableSigningProvider& keystore, CScript& script_out)
{
    // Gather public keys
    if (required < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "a multisignature address must require at least one key to redeem");
    }
    if ((int)pubkeys.size() < required) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("not enough keys supplied (got %u keys, but need at least %d to redeem)", pubkeys.size(), required));
    }
    if (pubkeys.size() > MAX_PUBKEYS_PER_MULTISIG) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Number of keys involved in the multisignature address creation > %d\nReduce the number", MAX_PUBKEYS_PER_MULTISIG));
    }

    script_out = GetScriptForMultisig(required, pubkeys);

    // Check if any keys are uncompressed. If so, the type is legacy
    for (const CPubKey& pk : pubkeys) {
        if (!pk.IsCompressed()) {
            type = OutputType::LEGACY;
            break;
        }
    }

    if (type == OutputType::LEGACY && script_out.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, (strprintf("redeemScript exceeds size limit: %d > %d", script_out.size(), MAX_SCRIPT_ELEMENT_SIZE)));
    }

    // Make the address
    CTxDestination dest = AddAndGetDestinationForScript(keystore, script_out, type);

    return dest;
}

class DescribeAddressVisitor
{
public:
    explicit DescribeAddressVisitor() {}

    UniValue operator()(const CNoDestination& dest) const
    {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const PKHash& keyID) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        obj.pushKV("iswitness", false);
        return obj;
    }

    UniValue operator()(const ScriptHash& scriptID) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", false);
        return obj;
    }

    UniValue operator()(const WitnessV0KeyHash& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 0);
        obj.pushKV("witness_program", HexStr(id));
        return obj;
    }

    UniValue operator()(const WitnessV0ScriptHash& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 0);
        obj.pushKV("witness_program", HexStr(id));
        return obj;
    }

    UniValue operator()(const WitnessV1Taproot& tap) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 1);
        obj.pushKV("witness_program", HexStr(tap));
        return obj;
    }

    UniValue operator()(const WitnessUnknown& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", (int)id.version);
        obj.pushKV("witness_program", HexStr(Span<const unsigned char>(id.program, id.length)));
        return obj;
    }
};

UniValue DescribeAddress(const CTxDestination& dest)
{
    return std::visit(DescribeAddressVisitor(), dest);
}

unsigned int ParseConfirmTarget(const UniValue& value, unsigned int max_target)
{
    const int target{value.get_int()};
    const unsigned int unsigned_target{static_cast<unsigned int>(target)};
    if (target < 1 || unsigned_target > max_target) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid conf_target, must be between %u and %u", 1, max_target));
    }
    return unsigned_target;
}

RPCErrorCode RPCErrorFromTransactionError(TransactionError terr)
{
    switch (terr) {
        case TransactionError::MEMPOOL_REJECTED:
            return RPC_TRANSACTION_REJECTED;
        case TransactionError::ALREADY_IN_CHAIN:
            return RPC_TRANSACTION_ALREADY_IN_CHAIN;
        case TransactionError::P2P_DISABLED:
            return RPC_CLIENT_P2P_DISABLED;
        case TransactionError::INVALID_PSBT:
        case TransactionError::PSBT_MISMATCH:
            return RPC_INVALID_PARAMETER;
        case TransactionError::SIGHASH_MISMATCH:
            return RPC_DESERIALIZATION_ERROR;
        default: break;
    }
    return RPC_TRANSACTION_ERROR;
}

UniValue JSONRPCTransactionError(TransactionError terr, const std::string& err_string)
{
    if (err_string.length() > 0) {
        return JSONRPCError(RPCErrorFromTransactionError(terr), err_string);
    } else {
        return JSONRPCError(RPCErrorFromTransactionError(terr), TransactionErrorString(terr).original);
    }
}

/**
 * A pair of strings that can be aligned (through padding) with other Sections
 * later on
 */
struct Section {
    Section(const std::string& left, const std::string& right)
        : m_left{left}, m_right{right} {}
    std::string m_left;
    const std::string m_right;
};

/**
 * Keeps track of RPCArgs by transforming them into sections for the purpose
 * of serializing everything to a single string
 */
struct Sections {
    std::vector<Section> m_sections;
    size_t m_max_pad{0};

    void PushSection(const Section& s)
    {
        m_max_pad = std::max(m_max_pad, s.m_left.size());
        m_sections.push_back(s);
    }

    /**
     * Recursive helper to translate an RPCArg into sections
     */
    void Push(const RPCArg& arg, const size_t current_indent = 5, const OuterType outer_type = OuterType::NONE)
    {
        const auto indent = std::string(current_indent, ' ');
        const auto indent_next = std::string(current_indent + 2, ' ');
        const bool push_name{outer_type == OuterType::OBJ}; // Dictionary keys must have a name

        switch (arg.m_type) {
        case RPCArg::Type::STR_HEX:
        case RPCArg::Type::STR:
        case RPCArg::Type::NUM:
        case RPCArg::Type::AMOUNT:
        case RPCArg::Type::RANGE:
        case RPCArg::Type::BOOL: {
            if (outer_type == OuterType::NONE) return; // Nothing more to do for non-recursive types on first recursion
            auto left = indent;
            if (arg.m_type_str.size() != 0 && push_name) {
                left += "\"" + arg.GetName() + "\": " + arg.m_type_str.at(0);
            } else {
                left += push_name ? arg.ToStringObj(/* oneline */ false) : arg.ToString(/* oneline */ false);
            }
            left += ",";
            PushSection({left, arg.ToDescriptionString()});
            break;
        }
        case RPCArg::Type::OBJ:
        case RPCArg::Type::OBJ_USER_KEYS: {
            const auto right = outer_type == OuterType::NONE ? "" : arg.ToDescriptionString();
            PushSection({indent + (push_name ? "\"" + arg.GetName() + "\": " : "") + "{", right});
            for (const auto& arg_inner : arg.m_inner) {
                Push(arg_inner, current_indent + 2, OuterType::OBJ);
            }
            if (arg.m_type != RPCArg::Type::OBJ) {
                PushSection({indent_next + "...", ""});
            }
            PushSection({indent + "}" + (outer_type != OuterType::NONE ? "," : ""), ""});
            break;
        }
        case RPCArg::Type::ARR: {
            auto left = indent;
            left += push_name ? "\"" + arg.GetName() + "\": " : "";
            left += "[";
            const auto right = outer_type == OuterType::NONE ? "" : arg.ToDescriptionString();
            PushSection({left, right});
            for (const auto& arg_inner : arg.m_inner) {
                Push(arg_inner, current_indent + 2, OuterType::ARR);
            }
            PushSection({indent_next + "...", ""});
            PushSection({indent + "]" + (outer_type != OuterType::NONE ? "," : ""), ""});
            break;
        }
        } // no default case, so the compiler can warn about missing cases
    }

    /**
     * Concatenate all sections with proper padding
     */
    std::string ToString() const
    {
        std::string ret;
        const size_t pad = m_max_pad + 4;
        for (const auto& s : m_sections) {
            // The left part of a section is assumed to be a single line, usually it is the name of the JSON struct or a
            // brace like {, }, [, or ]
            CHECK_NONFATAL(s.m_left.find('\n') == std::string::npos);
            if (s.m_right.empty()) {
                ret += s.m_left;
                ret += "\n";
                continue;
            }

            std::string left = s.m_left;
            left.resize(pad, ' ');
            ret += left;

            // Properly pad after newlines
            std::string right;
            size_t begin = 0;
            size_t new_line_pos = s.m_right.find_first_of('\n');
            while (true) {
                right += s.m_right.substr(begin, new_line_pos - begin);
                if (new_line_pos == std::string::npos) {
                    break; //No new line
                }
                right += "\n" + std::string(pad, ' ');
                begin = s.m_right.find_first_not_of(' ', new_line_pos + 1);
                if (begin == std::string::npos) {
                    break; // Empty line
                }
                new_line_pos = s.m_right.find_first_of('\n', begin + 1);
            }
            ret += right;
            ret += "\n";
        }
        return ret;
    }
};

RPCHelpMan::RPCHelpMan(std::string name, std::string description, std::vector<RPCArg> args, RPCResults results, RPCExamples examples)
    : RPCHelpMan{std::move(name), std::move(description), std::move(args), std::move(results), std::move(examples), nullptr} {}

RPCHelpMan::RPCHelpMan(std::string name, std::string description, std::vector<RPCArg> args, RPCResults results, RPCExamples examples, RPCMethodImpl fun)
    : m_name{std::move(name)},
      m_fun{std::move(fun)},
      m_description{std::move(description)},
      m_args{std::move(args)},
      m_results{std::move(results)},
      m_examples{std::move(examples)}
{
    std::set<std::string> named_args;
    for (const auto& arg : m_args) {
        std::vector<std::string> names;
        boost::split(names, arg.m_names, boost::is_any_of("|"));
        // Should have unique named arguments
        for (const std::string& name : names) {
            CHECK_NONFATAL(named_args.insert(name).second);
        }
        // Default value type should match argument type only when defined
        if (arg.m_fallback.index() == 2) {
            const RPCArg::Type type = arg.m_type;
            switch (std::get<RPCArg::Default>(arg.m_fallback).getType()) {
            case UniValue::VOBJ:
                CHECK_NONFATAL(type == RPCArg::Type::OBJ);
                break;
            case UniValue::VARR:
                CHECK_NONFATAL(type == RPCArg::Type::ARR);
                break;
            case UniValue::VSTR:
                CHECK_NONFATAL(type == RPCArg::Type::STR || type == RPCArg::Type::STR_HEX || type == RPCArg::Type::AMOUNT);
                break;
            case UniValue::VNUM:
                CHECK_NONFATAL(type == RPCArg::Type::NUM || type == RPCArg::Type::AMOUNT || type == RPCArg::Type::RANGE);
                break;
            case UniValue::VBOOL:
                CHECK_NONFATAL(type == RPCArg::Type::BOOL);
                break;
            case UniValue::VNULL:
                // Null values are accepted in all arguments
                break;
            default:
                CHECK_NONFATAL(false);
                break;
            }
        }
    }
}

std::string RPCResults::ToDescriptionString() const
{
    std::string result;
    for (const auto& r : m_results) {
        if (r.m_type == RPCResult::Type::ANY) continue; // for testing only
        if (r.m_cond.empty()) {
            result += "\nResult:\n";
        } else {
            result += "\nResult (" + r.m_cond + "):\n";
        }
        Sections sections;
        r.ToSections(sections);
        result += sections.ToString();
    }
    return result;
}

std::string RPCExamples::ToDescriptionString() const
{
    return m_examples.empty() ? m_examples : "\nExamples:\n" + m_examples;
}

UniValue RPCHelpMan::HandleRequest(const JSONRPCRequest& request) const
{
    if (request.mode == JSONRPCRequest::GET_ARGS) {
        return GetArgMap();
    }
    /*
     * Check if the given request is valid according to this command or if
     * the user is asking for help information, and throw help when appropriate.
     */
    if (request.mode == JSONRPCRequest::GET_HELP || !IsValidNumArgs(request.params.size())) {
        throw std::runtime_error(ToString());
    }
    const UniValue ret = m_fun(*this, request);
    CHECK_NONFATAL(std::any_of(m_results.m_results.begin(), m_results.m_results.end(), [ret](const RPCResult& res) { return res.MatchesType(ret); }));
    return ret;
}

bool RPCHelpMan::IsValidNumArgs(size_t num_args) const
{
    size_t num_required_args = 0;
    for (size_t n = m_args.size(); n > 0; --n) {
        if (!m_args.at(n - 1).IsOptional()) {
            num_required_args = n;
            break;
        }
    }
    return num_required_args <= num_args && num_args <= m_args.size();
}

std::vector<std::string> RPCHelpMan::GetArgNames() const
{
    std::vector<std::string> ret;
    for (const auto& arg : m_args) {
        ret.emplace_back(arg.m_names);
    }
    return ret;
}

std::string RPCHelpMan::ToString() const
{
    std::string ret;

    // Oneline summary
    ret += m_name;
    bool was_optional{false};
    for (const auto& arg : m_args) {
        if (arg.m_hidden) break; // Any arg that follows is also hidden
        const bool optional = arg.IsOptional();
        ret += " ";
        if (optional) {
            if (!was_optional) ret += "( ";
            was_optional = true;
        } else {
            if (was_optional) ret += ") ";
            was_optional = false;
        }
        ret += arg.ToString(/* oneline */ true);
    }
    if (was_optional) ret += " )";
    ret += "\n";

    // Description
    ret += m_description;

    // Arguments
    Sections sections;
    for (size_t i{0}; i < m_args.size(); ++i) {
        const auto& arg = m_args.at(i);
        if (arg.m_hidden) break; // Any arg that follows is also hidden

        if (i == 0) ret += "\nArguments:\n";

        // Push named argument name and description
        sections.m_sections.emplace_back(::ToString(i + 1) + ". " + arg.GetFirstName(), arg.ToDescriptionString());
        sections.m_max_pad = std::max(sections.m_max_pad, sections.m_sections.back().m_left.size());

        // Recursively push nested args
        sections.Push(arg);
    }
    ret += sections.ToString();

    // Result
    ret += m_results.ToDescriptionString();

    // Examples
    ret += m_examples.ToDescriptionString();

    return ret;
}

UniValue RPCHelpMan::GetArgMap() const
{
    UniValue arr{UniValue::VARR};
    for (int i{0}; i < int(m_args.size()); ++i) {
        const auto& arg = m_args.at(i);
        std::vector<std::string> arg_names;
        boost::split(arg_names, arg.m_names, boost::is_any_of("|"));
        for (const auto& arg_name : arg_names) {
            UniValue map{UniValue::VARR};
            map.push_back(m_name);
            map.push_back(i);
            map.push_back(arg_name);
            map.push_back(arg.m_type == RPCArg::Type::STR ||
                          arg.m_type == RPCArg::Type::STR_HEX);
            arr.push_back(map);
        }
    }
    return arr;
}

std::string RPCArg::GetFirstName() const
{
    return m_names.substr(0, m_names.find("|"));
}

std::string RPCArg::GetName() const
{
    CHECK_NONFATAL(std::string::npos == m_names.find("|"));
    return m_names;
}

bool RPCArg::IsOptional() const
{
    if (m_fallback.index() != 0) {
        return true;
    } else {
        return RPCArg::Optional::NO != std::get<RPCArg::Optional>(m_fallback);
    }
}

std::string RPCArg::ToDescriptionString() const
{
    std::string ret;
    ret += "(";
    if (m_type_str.size() != 0) {
        ret += m_type_str.at(1);
    } else {
        switch (m_type) {
        case Type::STR_HEX:
        case Type::STR: {
            ret += "string";
            break;
        }
        case Type::NUM: {
            ret += "numeric";
            break;
        }
        case Type::AMOUNT: {
            ret += "numeric or string";
            break;
        }
        case Type::RANGE: {
            ret += "numeric or array";
            break;
        }
        case Type::BOOL: {
            ret += "boolean";
            break;
        }
        case Type::OBJ:
        case Type::OBJ_USER_KEYS: {
            ret += "json object";
            break;
        }
        case Type::ARR: {
            ret += "json array";
            break;
        }
        } // no default case, so the compiler can warn about missing cases
    }
    if (m_fallback.index() == 1) {
        ret += ", optional, default=" + std::get<RPCArg::DefaultHint>(m_fallback);
    } else if (m_fallback.index() == 2) {
        ret += ", optional, default=" + std::get<RPCArg::Default>(m_fallback).write();
    } else {
        switch (std::get<RPCArg::Optional>(m_fallback)) {
        case RPCArg::Optional::OMITTED: {
            // nothing to do. Element is treated as if not present and has no default value
            break;
        }
        case RPCArg::Optional::OMITTED_NAMED_ARG: {
            ret += ", optional"; // Default value is "null"
            break;
        }
        case RPCArg::Optional::NO: {
            ret += ", required";
            break;
        }
        } // no default case, so the compiler can warn about missing cases
    }
    ret += ")";
    ret += m_description.empty() ? "" : " " + m_description;
    return ret;
}

void RPCResult::ToSections(Sections& sections, const OuterType outer_type, const int current_indent) const
{
    // Indentation
    const std::string indent(current_indent, ' ');
    const std::string indent_next(current_indent + 2, ' ');

    // Elements in a JSON structure (dictionary or array) are separated by a comma
    const std::string maybe_separator{outer_type != OuterType::NONE ? "," : ""};

    // The key name if recursed into an dictionary
    const std::string maybe_key{
        outer_type == OuterType::OBJ ?
            "\"" + this->m_key_name + "\" : " :
            ""};

    // Format description with type
    const auto Description = [&](const std::string& type) {
        return "(" + type + (this->m_optional ? ", optional" : "") + ")" +
               (this->m_description.empty() ? "" : " " + this->m_description);
    };

    switch (m_type) {
    case Type::ELISION: {
        // If the inner result is empty, use three dots for elision
        sections.PushSection({indent + "..." + maybe_separator, m_description});
        return;
    }
    case Type::ANY: {
        CHECK_NONFATAL(false); // Only for testing
    }
    case Type::NONE: {
        sections.PushSection({indent + "null" + maybe_separator, Description("json null")});
        return;
    }
    case Type::STR: {
        sections.PushSection({indent + maybe_key + "\"str\"" + maybe_separator, Description("string")});
        return;
    }
    case Type::STR_AMOUNT: {
        sections.PushSection({indent + maybe_key + "n" + maybe_separator, Description("numeric")});
        return;
    }
    case Type::STR_HEX: {
        sections.PushSection({indent + maybe_key + "\"hex\"" + maybe_separator, Description("string")});
        return;
    }
    case Type::NUM: {
        sections.PushSection({indent + maybe_key + "n" + maybe_separator, Description("numeric")});
        return;
    }
    case Type::NUM_TIME: {
        sections.PushSection({indent + maybe_key + "xxx" + maybe_separator, Description("numeric")});
        return;
    }
    case Type::BOOL: {
        sections.PushSection({indent + maybe_key + "true|false" + maybe_separator, Description("boolean")});
        return;
    }
    case Type::ARR_FIXED:
    case Type::ARR: {
        sections.PushSection({indent + maybe_key + "[", Description("json array")});
        for (const auto& i : m_inner) {
            i.ToSections(sections, OuterType::ARR, current_indent + 2);
        }
        CHECK_NONFATAL(!m_inner.empty());
        if (m_type == Type::ARR && m_inner.back().m_type != Type::ELISION) {
            sections.PushSection({indent_next + "...", ""});
        } else {
            // Remove final comma, which would be invalid JSON
            sections.m_sections.back().m_left.pop_back();
        }
        sections.PushSection({indent + "]" + maybe_separator, ""});
        return;
    }
    case Type::OBJ_DYN:
    case Type::OBJ: {
        sections.PushSection({indent + maybe_key + "{", Description("json object")});
        for (const auto& i : m_inner) {
            i.ToSections(sections, OuterType::OBJ, current_indent + 2);
        }
        CHECK_NONFATAL(!m_inner.empty());
        if (m_type == Type::OBJ_DYN && m_inner.back().m_type != Type::ELISION) {
            // If the dictionary keys are dynamic, use three dots for continuation
            sections.PushSection({indent_next + "...", ""});
        } else {
            // Remove final comma, which would be invalid JSON
            sections.m_sections.back().m_left.pop_back();
        }
        sections.PushSection({indent + "}" + maybe_separator, ""});
        return;
    }
    } // no default case, so the compiler can warn about missing cases
    CHECK_NONFATAL(false);
}

bool RPCResult::MatchesType(const UniValue& result) const
{
    switch (m_type) {
    case Type::ELISION: {
        return false;
    }
    case Type::ANY: {
        return true;
    }
    case Type::NONE: {
        return UniValue::VNULL == result.getType();
    }
    case Type::STR:
    case Type::STR_HEX: {
        return UniValue::VSTR == result.getType();
    }
    case Type::NUM:
    case Type::STR_AMOUNT:
    case Type::NUM_TIME: {
        return UniValue::VNUM == result.getType();
    }
    case Type::BOOL: {
        return UniValue::VBOOL == result.getType();
    }
    case Type::ARR_FIXED:
    case Type::ARR: {
        return UniValue::VARR == result.getType();
    }
    case Type::OBJ_DYN:
    case Type::OBJ: {
        return UniValue::VOBJ == result.getType();
    }
    } // no default case, so the compiler can warn about missing cases
    CHECK_NONFATAL(false);
}

std::string RPCArg::ToStringObj(const bool oneline) const
{
    std::string res;
    res += "\"";
    res += GetFirstName();
    if (oneline) {
        res += "\":";
    } else {
        res += "\": ";
    }
    switch (m_type) {
    case Type::STR:
        return res + "\"str\"";
    case Type::STR_HEX:
        return res + "\"hex\"";
    case Type::NUM:
        return res + "n";
    case Type::RANGE:
        return res + "n or [n,n]";
    case Type::AMOUNT:
        return res + "amount";
    case Type::BOOL:
        return res + "bool";
    case Type::ARR:
        res += "[";
        for (const auto& i : m_inner) {
            res += i.ToString(oneline) + ",";
        }
        return res + "...]";
    case Type::OBJ:
    case Type::OBJ_USER_KEYS:
        // Currently unused, so avoid writing dead code
        CHECK_NONFATAL(false);
    } // no default case, so the compiler can warn about missing cases
    CHECK_NONFATAL(false);
}

std::string RPCArg::ToString(const bool oneline) const
{
    if (oneline && !m_oneline_description.empty()) return m_oneline_description;

    switch (m_type) {
    case Type::STR_HEX:
    case Type::STR: {
        return "\"" + GetFirstName() + "\"";
    }
    case Type::NUM:
    case Type::RANGE:
    case Type::AMOUNT:
    case Type::BOOL: {
        return GetFirstName();
    }
    case Type::OBJ:
    case Type::OBJ_USER_KEYS: {
        const std::string res = Join(m_inner, ",", [&](const RPCArg& i) { return i.ToStringObj(oneline); });
        if (m_type == Type::OBJ) {
            return "{" + res + "}";
        } else {
            return "{" + res + ",...}";
        }
    }
    case Type::ARR: {
        std::string res;
        for (const auto& i : m_inner) {
            res += i.ToString(oneline) + ",";
        }
        return "[" + res + "...]";
    }
    } // no default case, so the compiler can warn about missing cases
    CHECK_NONFATAL(false);
}

static std::pair<int64_t, int64_t> ParseRange(const UniValue& value)
{
    if (value.isNum()) {
        return {0, value.get_int64()};
    }
    if (value.isArray() && value.size() == 2 && value[0].isNum() && value[1].isNum()) {
        int64_t low = value[0].get_int64();
        int64_t high = value[1].get_int64();
        if (low > high) throw JSONRPCError(RPC_INVALID_PARAMETER, "Range specified as [begin,end] must not have begin after end");
        return {low, high};
    }
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Range must be specified as end or as [begin,end]");
}

std::pair<int64_t, int64_t> ParseDescriptorRange(const UniValue& value)
{
    int64_t low, high;
    std::tie(low, high) = ParseRange(value);
    if (low < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range should be greater or equal than 0");
    }
    if ((high >> 31) != 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "End of range is too high");
    }
    if (high >= low + 1000000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range is too large");
    }
    return {low, high};
}

std::vector<CScript> EvalDescriptorStringOrObject(const UniValue& scanobject, FlatSigningProvider& provider)
{
    std::string desc_str;
    std::pair<int64_t, int64_t> range = {0, 1000};
    if (scanobject.isStr()) {
        desc_str = scanobject.get_str();
    } else if (scanobject.isObject()) {
        UniValue desc_uni = find_value(scanobject, "desc");
        if (desc_uni.isNull()) throw JSONRPCError(RPC_INVALID_PARAMETER, "Descriptor needs to be provided in scan object");
        desc_str = desc_uni.get_str();
        UniValue range_uni = find_value(scanobject, "range");
        if (!range_uni.isNull()) {
            range = ParseDescriptorRange(range_uni);
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Scan object needs to be either a string or an object");
    }

    std::string error;
    auto desc = Parse(desc_str, provider, error);
    if (!desc) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, error);
    }
    if (!desc->IsRange()) {
        range.first = 0;
        range.second = 0;
    }
    std::vector<CScript> ret;
    for (int i = range.first; i <= range.second; ++i) {
        std::vector<CScript> scripts;
        if (!desc->Expand(i, provider, scripts, provider)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Cannot derive script without private keys: '%s'", desc_str));
        }
        std::move(scripts.begin(), scripts.end(), std::back_inserter(ret));
    }
    return ret;
}

UniValue GetServicesNames(ServiceFlags services)
{
    UniValue servicesNames(UniValue::VARR);

    for (const auto& flag : serviceFlagsToStr(services)) {
        servicesNames.push_back(flag);
    }

    return servicesNames;
}
