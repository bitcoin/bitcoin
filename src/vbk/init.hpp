#ifndef BITCOIN_SRC_VBK_INIT_HPP
#define BITCOIN_SRC_VBK_INIT_HPP

#include "pop_service_impl.hpp"
#include "vbk/config.hpp"
#include "vbk/service_locator.hpp"
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
    return detail::InitService<PopService, PopServiceImpl>(Params().GetPopConfig());
}

inline Config& InitConfig()
{
    auto* cfg = new Config();
    setService<Config>(cfg);
    return *cfg;
}


} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_INIT_HPP
