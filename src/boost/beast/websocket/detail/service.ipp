//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_SERVICE_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_SERVICE_IPP

#include <boost/beast/websocket/detail/service.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

service::
impl_type::
impl_type(net::execution_context& ctx)
    : svc_(net::use_service<service>(ctx))
{
    std::lock_guard<std::mutex> g(svc_.m_);
    index_ = svc_.v_.size();
    svc_.v_.push_back(this);
}

void
service::
impl_type::
remove()
{
    std::lock_guard<std::mutex> g(svc_.m_);
    auto& other = *svc_.v_.back();
    other.index_ = index_;
    svc_.v_[index_] = &other;
    svc_.v_.pop_back();
}

//---

void
service::
shutdown()
{
    std::vector<boost::weak_ptr<impl_type>> v;
    {
        std::lock_guard<std::mutex> g(m_);
        v.reserve(v_.size());
        for(auto p : v_)
            v.emplace_back(p->weak_from_this());
    }
    for(auto wp : v)
        if(auto sp = wp.lock())
            sp->shutdown();
}

} // detail
} // websocket
} // beast
} // boost

#endif
