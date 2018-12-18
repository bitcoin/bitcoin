// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <keystore.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>

InitInterfaces* g_rpc_interfaces = nullptr;

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

// Retrieves a public key for an address from the given CKeyStore
CPubKey AddrToPubKey(CKeyStore* const keystore, const std::string& addr_in)
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

// Creates a multisig redeemscript from a given list of public keys and number required.
CScript CreateMultisigRedeemscript(const int required, const std::vector<CPubKey>& pubkeys)
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

    CScript result = GetScriptForMultisig(required, pubkeys);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, (strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE)));
    }

    return result;
}

class DescribeAddressVisitor : public boost::static_visitor<UniValue>
{
public:
    explicit DescribeAddressVisitor() {}

    UniValue operator()(const CNoDestination& dest) const
    {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const CKeyID& keyID) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        obj.pushKV("iswitness", false);
        return obj;
    }

    UniValue operator()(const CScriptID& scriptID) const
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

struct Section {
    Section(const std::string& left, const std::string& right)
        : m_left{left}, m_right{right} {}
    const std::string m_left;
    const std::string m_right;
};

struct Sections {
    std::vector<Section> m_sections;
    size_t m_max_pad{0};

    void PushSection(const Section& s)
    {
        m_max_pad = std::max(m_max_pad, s.m_left.size());
        m_sections.push_back(s);
    }

    enum class OuterType {
        ARR,
        OBJ,
        NAMED_ARG, // Only set on first recursion
    };

    void Push(const RPCArg& arg, const size_t current_indent = 5, const OuterType outer_type = OuterType::NAMED_ARG)
    {
        const auto indent = std::string(current_indent, ' ');
        const auto indent_next = std::string(current_indent + 2, ' ');
        switch (arg.m_type) {
        case RPCArg::Type::STR_HEX:
        case RPCArg::Type::STR:
        case RPCArg::Type::NUM:
        case RPCArg::Type::AMOUNT:
        case RPCArg::Type::BOOL: {
            if (outer_type == OuterType::NAMED_ARG) return; // Nothing more to do for non-recursive types on first recursion
            auto left = indent;
            if (arg.m_type_str.size() != 0 && outer_type == OuterType::OBJ) {
                left += "\"" + arg.m_name + "\": " + arg.m_type_str.at(0);
            } else {
                left += outer_type == OuterType::OBJ ? arg.ToStringObj(/* oneline */ false) : arg.ToString(/* oneline */ false);
            }
            left += ",";
            PushSection({left, arg.ToDescriptionString(/* implicitly_required */ outer_type == OuterType::ARR)});
            break;
        }
        case RPCArg::Type::OBJ:
        case RPCArg::Type::OBJ_USER_KEYS: {
            const auto right = outer_type == OuterType::NAMED_ARG ? "" : arg.ToDescriptionString(/* implicitly_required */ outer_type == OuterType::ARR);
            PushSection({indent + "{", right});
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
            left += outer_type == OuterType::OBJ ? "\"" + arg.m_name + "\": " : "";
            left += "[";
            const auto right = outer_type == OuterType::NAMED_ARG ? "" : arg.ToDescriptionString(/* implicitly_required */ outer_type == OuterType::ARR);
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

RPCHelpMan::RPCHelpMan(const std::string& name, const std::string& description, const std::vector<RPCArg>& args)
    : m_name{name}, m_description{description}, m_args{args}
{
    std::set<std::string> named_args;
    for (const auto& arg : m_args) {
        // Should have unique named arguments
        assert(named_args.insert(arg.m_name).second);
    }
}

std::string RPCHelpMan::ToString() const
{
    std::string ret;

    // Oneline summary
    ret += m_name;
    bool was_optional{false};
    for (const auto& arg : m_args) {
        ret += " ";
        if (arg.m_optional) {
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

    return ret;
}

std::string RPCArg::ToDescriptionString(const bool implicitly_required) const
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
    if (!implicitly_required) {
        ret += ", ";
        if (m_optional) {
            ret += "optional";
            if (!m_default_value.empty()) {
                ret += ", default=" + m_default_value;
            } else {
                // TODO enable this assert, when all optional parameters have their default value documented
                //assert(false);
            }
        } else {
            ret += "required";
            assert(m_default_value.empty()); // Default value is ignored, and must not be present
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
    case Type::AMOUNT:
    case Type::BOOL: {
        return m_name;
    }
    case Type::OBJ:
    case Type::OBJ_USER_KEYS: {
        std::string res;
        for (size_t i = 0; i < m_inner.size();) {
            res += m_inner[i].ToStringObj(oneline);
            if (++i < m_inner.size()) res += ",";
        }
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
