#ifndef BITCOIN_SRC_VBK_RPC_HPP
#define BITCOIN_SRC_VBK_RPC_HPP

class UniValue;
class JSONRPCRequest;

namespace VeriBlock {

struct RpcService {
    virtual ~RpcService() = default;

    virtual UniValue submitpop(const JSONRPCRequest& request) = 0;

    virtual UniValue getpopdata(const JSONRPCRequest& request) = 0;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_RPC_HPP
