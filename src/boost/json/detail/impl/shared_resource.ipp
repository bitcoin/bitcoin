//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_SHARED_RESOURCE_IPP
#define BOOST_JSON_DETAIL_IMPL_SHARED_RESOURCE_IPP

#include <boost/json/detail/shared_resource.hpp>

BOOST_JSON_NS_BEGIN
namespace detail {

// these are here so that ~memory_resource
// is emitted in the library instead of
// the user's TU.

shared_resource::
shared_resource()
{
}

shared_resource::
~shared_resource()
{
}

} // detail
BOOST_JSON_NS_END

#endif
