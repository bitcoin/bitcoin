//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_SAVED_HANDLER_IPP
#define BOOST_BEAST_CORE_IMPL_SAVED_HANDLER_IPP

#include <boost/beast/core/saved_handler.hpp>
#include <boost/core/exchange.hpp>

namespace boost {
namespace beast {

saved_handler::
~saved_handler()
{
    if(p_)
        p_->destroy();
}

saved_handler::
saved_handler(saved_handler&& other) noexcept
    : p_(boost::exchange(other.p_, nullptr))
{
}

saved_handler&
saved_handler::
operator=(saved_handler&& other) noexcept
{
    // Can't delete a handler before invoking
    BOOST_ASSERT(! has_value());
    p_ = boost::exchange(other.p_, nullptr);
    return *this;
}

bool
saved_handler::
reset() noexcept
{
    if(! p_)
        return false;
    boost::exchange(p_, nullptr)->destroy();
    return true;
}

void
saved_handler::
invoke()
{
    // Can't invoke without a value
    BOOST_ASSERT(has_value());
    boost::exchange(
        p_, nullptr)->invoke();
}

bool
saved_handler::
maybe_invoke()
{
    if(! p_)
        return false;
    boost::exchange(
        p_, nullptr)->invoke();
    return true;
}

} // beast
} // boost

#endif
