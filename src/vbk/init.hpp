#ifndef BITCOIN_SRC_VBK_INIT_HPP
#define BITCOIN_SRC_VBK_INIT_HPP

#include "vbk/config.hpp"
#include "vbk/pop_service/pop_service_impl.hpp"
#include "vbk/rpc_service/rpc_service_impl.hpp"
#include "vbk/service_locator.hpp"
#include "vbk/util_service/util_service_impl.hpp"
#include <util/system.h>

#include <utility>

namespace VeriBlock {

namespace detail {
template <typename Face, typename Impl>
Face& InitService()
{
    auto* ptr = new Impl();
    setService<Face>(ptr);
    return *ptr;
}

template <typename Face, typename Impl, typename Arg1>
Face& InitService(Arg1 arg)
{
    auto* ptr = new Impl(arg);
    setService<Face>(ptr);
    return *ptr;
}
} // namespace detail


inline PopService& InitPopService()
{
    return detail::InitService<PopService, PopServiceImpl>();
}

inline UtilService& InitUtilService()
{
    return detail::InitService<UtilService, UtilServiceImpl>();
}

inline RpcService& InitRpcService(CConnman* connman)
{
    assert(connman != nullptr && "connman is nullptr");
    return detail::InitService<RpcService, RpcServiceImpl>(connman);
}

inline Config& InitConfig()
{
    auto* cfg = new Config();
    setService<Config>(cfg);
    return *cfg;
}

inline PopService& InitPopService(std::string host, std::string port, bool altautoconfig)
{
    auto& config = getService<Config>();
    config.service_ip = std::move(host);
    config.service_port = std::move(port);

    return detail::InitService<PopService, PopServiceImpl>(altautoconfig);
}


inline void ResetVeriBlock()
{
    setService<Config>(nullptr);
    setService<UtilService>(nullptr);
    setService<PopService>(nullptr);
    setService<RpcService>(nullptr);
}

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_INIT_HPP
