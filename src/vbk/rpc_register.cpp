#include "rpc_register.hpp"

#include "vbk/rpc_service.hpp"
#include "vbk/service_locator.hpp"
#include <rpc/server.h>

namespace VeriBlock {

namespace {

UniValue getpopdata(const JSONRPCRequest& request)
{
    auto& rpc = VeriBlock::getService<VeriBlock::RpcService>();
    return rpc.getpopdata(request);
}

UniValue submitpop(const JSONRPCRequest& request)
{
    auto& rpc = VeriBlock::getService<VeriBlock::RpcService>();
    return rpc.submitpop(request);
}

UniValue updatecontext(const JSONRPCRequest& request)
{
    auto& rpc = VeriBlock::getService<VeriBlock::RpcService>();
    return rpc.updatecontext(request);
}

const CRPCCommand commands[] = {
    {"pop_mining", "submitpop", &submitpop, {"atv", "vtbs"}},
    {"pop_mining", "getpopdata", &getpopdata, {"blockhash"}},
    {"pop_mining", "updatecontext", &updatecontext, {"string"}},
};

} // namespace

void RegisterPOPMiningRPCCommands(CRPCTable& t)
{
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}


} // namespace VeriBlock
