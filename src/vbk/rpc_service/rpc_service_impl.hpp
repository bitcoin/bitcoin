#ifndef BITCOIN_SRC_VBK_RPC_SERVICE_RPC_SERVICE_IMPL_HPP
#define BITCOIN_SRC_VBK_RPC_SERVICE_RPC_SERVICE_IMPL_HPP

#include <memory>

#include "vbk/util_service.hpp"
#include "vbk/pop_service.hpp"
#include "vbk/rpc_service.hpp"

class CWallet;

namespace VeriBlock {

class RpcServiceImpl : public RpcService
{
public:
    ~RpcServiceImpl() override = default;

    UniValue submitpop(const JSONRPCRequest& request) override;

    UniValue getpopdata(const JSONRPCRequest& request) override;

    UniValue updatecontext(const JSONRPCRequest& request) override;

private:
    UniValue doGetPopData(int height, const std::shared_ptr<CWallet>& wallet);

    UniValue doSubmitPop(const std::vector<uint8_t>& atv,
        const std::vector<std::vector<uint8_t>>& vtbs);
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_RPC_SERVICE_RPC_SERVICE_IMPL_HPP
