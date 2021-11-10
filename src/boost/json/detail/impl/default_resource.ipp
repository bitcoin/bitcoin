//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_DEFAULT_RESOURCE_IPP
#define BOOST_JSON_DETAIL_IMPL_DEFAULT_RESOURCE_IPP

#include <boost/json/detail/default_resource.hpp>

BOOST_JSON_NS_BEGIN
namespace detail {

#ifndef BOOST_JSON_WEAK_CONSTINIT
# ifndef BOOST_JSON_NO_DESTROY
BOOST_JSON_REQUIRE_CONST_INIT
default_resource::holder
default_resource::instance_;
# else
BOOST_JSON_REQUIRE_CONST_INIT
default_resource
default_resource::instance_;
# endif
#endif

// this is here so that ~memory_resource
// is emitted in the library instead of
// the user's TU.
default_resource::
~default_resource() = default;

void*
default_resource::
do_allocate(
    std::size_t n,
    std::size_t)
{
    return ::operator new(n);
}

void
default_resource::
do_deallocate(
    void* p,
    std::size_t,
    std::size_t)
{
    ::operator delete(p);
}

bool
default_resource::
do_is_equal(
    memory_resource const& mr) const noexcept
{
    return this == &mr;
}

} // detail
BOOST_JSON_NS_END

#endif
