//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_STACK_IPP
#define BOOST_JSON_DETAIL_IMPL_STACK_IPP

#include <boost/json/detail/stack.hpp>

BOOST_JSON_NS_BEGIN
namespace detail {

stack::
~stack()
{
    if(buf_)
        sp_->deallocate(
            buf_, cap_);
}

void
stack::
reserve(std::size_t n)
{
    if(cap_ >= n)
        return;
    auto const buf = static_cast<
        char*>(sp_->allocate(n));
    if(buf_)
    {
        if(size_ > 0)
            std::memcpy(buf, buf_, size_);
        sp_->deallocate(buf_, cap_);
    }
    buf_ = buf;
    cap_ = n;
}

} // detail
BOOST_JSON_NS_END

#endif
