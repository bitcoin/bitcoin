// src/rpc/ecai.cpp

#ifdef ENABLE_ECAI

#include <ecai/state.h>
#include <rpc/server.h>
#include <util/strencodings.h>

static RPCHelpMan ecai_loadpolicy()
{
    return RPCHelpMan{
        "ecai.loadpolicy",
        "\nLoad/clear ECAI policy TLV hex (empty string to clear).\n",
        {{"hex", RPCArg::Type::STR, RPCArg::Optional::NO, "Policy TLV hex or empty"}},
        RPCResult{
            "{ enabled: bool, policy_id: hex, c_pol: hex }\n"
        },
        RPCExamples{},
        [](const RPCHelpMan&, const JSONRPCRequest& req) -> UniValue {
            std::string hex = req.params[0].get_str();
            std::string err;
            if (!ecai::LoadPolicyHex(hex, err)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, err);
            }
            ecai::Enable(!hex.empty());

            UniValue o(UniValue::VOBJ);
            o.pushKV("enabled", ecai::Enabled());
            if (!hex.empty()) {
                o.pushKV("policy_id", ecai::g_state.policy.id.GetHex());
                o.pushKV("c_pol", HexStr(ecai::g_state.policy.c_pol));
            }
            return o;
        }
    };
}

void RegisterECAIRPCCommands(CRPCTable& t)
{
    static const CRPCCommand cmds[] = {
        {"ecai", &ecai_loadpolicy},
    };
    for (const auto& c : cmds) {
        t.appendCommand(c.name, &c);
    }
}

#endif // ENABLE_ECAI

