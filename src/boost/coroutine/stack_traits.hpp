
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_STACK_TRAITS_H
#define BOOST_COROUTINES_STACK_TRAITS_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

struct BOOST_COROUTINES_DECL stack_traits
{
    static bool is_unbounded() BOOST_NOEXCEPT;

    static std::size_t page_size() BOOST_NOEXCEPT;

    static std::size_t default_size() BOOST_NOEXCEPT;

    static std::size_t minimum_size() BOOST_NOEXCEPT;

    static std::size_t maximum_size() BOOST_NOEXCEPT;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_STACK_TRAITS_H
