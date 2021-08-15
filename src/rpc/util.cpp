// Copyright (c) 2017-2019 The Bitcoin Core developers
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

#include <tuple>

InitInterfaces* g_rpc_interfaces = nullptr;

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

CAmount AmountFromValue(const UniValue& value)
{
    if (!value.isNum() && !value.isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount))
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

std::string HelpExampleCli(const std::string& methodname, const std::string& args)
{
    return "> qitcoin-cli " + methodname + " " + args + "\n";
}

std::string HelpExampleRpc(const std::string& methodname, const std::string& args)
{
    return "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
        "\"method\": \"" + methodname + "\", \"params\": [" + args + "] }' -H 'content-type: text/plain;' http://127.0.0.1:3332/\n";
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
CPubKey AddrToPubKey(FillableSigningProvider* const keystore, const std::string& addr_in)
{
    CTxDestination dest = DecodeDestination(addr_in);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address: " + addr_in);
    }
    CKeyID key = GetKeyForDestination(*keystore, dest);
    if (key.IsNull()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("%s does not refer to a key", addr_in));
    }
    CPubKey vchPubKey;
    if (!keystore->GetPubKey(key, vchPubKey)) {
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
    if (pubkeys.size() > 16) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Number of keys involved in the multisignature address creation > 16\nReduce the number");
    }

    script_out = GetScriptForMultisig(required, pubkeys);

    if (script_out.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, (strprintf("redeemScript exceeds size limit: %d > %d", script_out.size(), MAX_SCRIPT_ELEMENT_SIZE)));
    }

    // Check if any keys are uncompressed. If so, the type is legacy
    for (const CPubKey& pk : pubkeys) {
        if (!pk.IsCompressed()) {
            type = OutputType::LEGACY;
            break;
        }
    }

    // Make the address
    CTxDestination dest = AddAndGetDestinationForScript(keystore, script_out, type);

    return dest;
}

class DescribeAddressVisitor : public boost::static_visitor<UniValue>
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
        obj.pushKV("witness_program", HexStr(id.begin(), id.end()));
        return obj;
    }

    UniValue operator()(const WitnessV0ScriptHash& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 0);
        obj.pushKV("witness_program", HexStr(id.begin(), id.end()));
        return obj;
    }

    UniValue operator()(const WitnessUnknown& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", (int)id.version);
        obj.pushKV("witness_program", HexStr(id.program, id.program + id.length));
        return obj;
    }
};

UniValue DescribeAddress(const CTxDestination& dest)
{
    return boost::apply_visitor(DescribeAddressVisitor(), dest);
}

unsigned int ParseConfirmTarget(const UniValue& value, unsigned int max_target)
{
    int target = value.get_int();
    if (target < 1 || (unsigned int)target > max_target) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid conf_target, must be between %u - %u", 1, max_target));
    }
    return (unsigned int)target;
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
        return JSONRPCError(RPCErrorFromTransactionError(terr), TransactionErrorString(terr));
    }
}

/**
 * A pair of strings that can be aligned (through padding) with other Sections
 * later on
 */
struct Section {
    Section(const std::string& left, const std::string& right)
        : m_left{left}, m_right{right} {}
    const std::string m_left;
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
     * Serializing RPCArgs depends on the outer type. Only arrays and
     * dictionaries can be nested in json. The top-level outer type is "named
     * arguments", a mix between a dictionary and arrays.
     */
    enum class OuterType {
        ARR,
        OBJ,
        NAMED_ARG, // Only set on first recursion
    };

    /**
     * Recursive helper to translate an RPCArg into sections
     */
    void Push(const RPCArg& arg, const size_t current_indent = 5, const OuterType outer_type = OuterType::NAMED_ARG)
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
            if (outer_type == OuterType::NAMED_ARG) return; // Nothing more to do for non-recursive types on first recursion
            auto left = indent;
            if (arg.m_type_str.size() != 0 && push_name) {
                left += "\"" + arg.m_name + "\": " + arg.m_type_str.at(0);
            } else {
                left += push_name ? arg.ToStringObj(/* oneline */ false) : arg.ToString(/* oneline */ false);
            }
            left += ",";
            PushSection({left, arg.ToDescriptionString()});
            break;
        }
        case RPCArg::Type::OBJ:
        case RPCArg::Type::OBJ_USER_KEYS: {
            const auto right = outer_type == OuterType::NAMED_ARG ? "" : arg.ToDescriptionString();
            PushSection({indent + (push_name ? "\"" + arg.m_name + "\": " : "") + "{", right});
            for (const auto& arg_inner : arg.m_inner) {
                Push(arg_inner, current_indent + 2, OuterType::OBJ);
            }
            if (arg.m_type != RPCArg::Type::OBJ) {
                PushSection({indent_next + "...", ""});
            }
            PushSection({indent + "}" + (outer_type != OuterType::NAMED_ARG ? "," : ""), ""});
            break;
        }
        case RPCArg::Type::ARR: {
            auto left = indent;
            left += push_name ? "\"" + arg.m_name + "\": " : "";
            left += "[";
            const auto right = outer_type == OuterType::NAMED_ARG ? "" : arg.ToDescriptionString();
            PushSection({left, right});
            for (const auto& arg_inner : arg.m_inner) {
                Push(arg_inner, current_indent + 2, OuterType::ARR);
            }
            PushSection({indent_next + "...", ""});
            PushSection({indent + "]" + (outer_type != OuterType::NAMED_ARG ? "," : ""), ""});
            break;
        }

            // no default case, so the compiler can warn about missing cases
        }
    }

    /**
     * Concatenate all sections with proper padding
     */
    std::string ToString() const
    {
        std::string ret;
        const size_t pad = m_max_pad + 4;
        for (const auto& s : m_sections) {
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
    : m_name{std::move(name)},
      m_description{std::move(description)},
      m_args{std::move(args)},
      m_results{std::move(results)},
      m_examples{std::move(examples)}
{
    std::set<std::string> named_args;
    for (const auto& arg : m_args) {
        // Should have unique named arguments
        assert(named_args.insert(arg.m_name).second);
    }
}

std::string RPCResults::ToDescriptionString() const
{
    std::string result;
    for (const auto& r : m_results) {
        if (r.m_cond.empty()) {
            result += "\nResult:\n";
        } else {
            result += "\nResult (" + r.m_cond + "):\n";
        }
        result += r.m_result;
    }
    return result;
}

std::string RPCExamples::ToDescriptionString() const
{
    return m_examples.empty() ? m_examples : "\nExamples:\n" + m_examples;
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
std::string RPCHelpMan::ToString() const
{
    std::string ret;

    // Oneline summary
    ret += m_name;
    bool was_optional{false};
    for (const auto& arg : m_args) {
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

        if (i == 0) ret += "\nArguments:\n";

        // Push named argument name and description
        sections.m_sections.emplace_back(std::to_string(i + 1) + ". " + arg.m_name, arg.ToDescriptionString());
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

bool RPCArg::IsOptional() const
{
    if (m_fallback.which() == 1) {
        return true;
    } else {
        return RPCArg::Optional::NO != boost::get<RPCArg::Optional>(m_fallback);
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

            // no default case, so the compiler can warn about missing cases
        }
    }
    if (m_fallback.which() == 1) {
        ret += ", optional, default=" + boost::get<std::string>(m_fallback);
    } else {
        switch (boost::get<RPCArg::Optional>(m_fallback)) {
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

            // no default case, so the compiler can warn about missing cases
        }
    }
    ret += ")";
    ret += m_description.empty() ? "" : " " + m_description;
    return ret;
}

std::string RPCArg::ToStringObj(const bool oneline) const
{
    std::string res;
    res += "\"";
    res += m_name;
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
        assert(false);

        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}

std::string RPCArg::ToString(const bool oneline) const
{
    if (oneline && !m_oneline_description.empty()) return m_oneline_description;

    switch (m_type) {
    case Type::STR_HEX:
    case Type::STR: {
        return "\"" + m_name + "\"";
    }
    case Type::NUM:
    case Type::RANGE:
    case Type::AMOUNT:
    case Type::BOOL: {
        return m_name;
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

        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
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

    if (services & NODE_NETWORK)
        servicesNames.push_back("NETWORK");
    if (services & NODE_GETUTXO)
        servicesNames.push_back("GETUTXO");
    if (services & NODE_BLOOM)
        servicesNames.push_back("BLOOM");
    if (services & NODE_WITNESS)
        servicesNames.push_back("WITNESS");
    if (services & NODE_NETWORK_LIMITED)
        servicesNames.push_back("NETWORK_LIMITED");

    return servicesNames;
}
