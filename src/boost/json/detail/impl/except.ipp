//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_EXCEPT_IPP
#define BOOST_JSON_DETAIL_IMPL_EXCEPT_IPP

#include <boost/json/detail/except.hpp>
#ifndef BOOST_JSON_STANDALONE
# include <boost/version.hpp>
# include <boost/throw_exception.hpp>
#elif defined(BOOST_JSON_STANDALONE) && defined(BOOST_NO_EXCEPTIONS)
# include <exception>
#endif
#include <stdexcept>

#if defined(BOOST_JSON_STANDALONE)
namespace boost {

#if defined(BOOST_NO_EXCEPTIONS)
// When exceptions are disabled
// in standalone, you must provide
// this function.
BOOST_NORETURN
void
throw_exception(std::exception const&);
#endif

} // boost
#endif

BOOST_JSON_NS_BEGIN

namespace detail {

#if defined(BOOST_JSON_STANDALONE) && \
    ! defined(BOOST_NO_EXCEPTIONS)
// this is in the json namespace to avoid
// colliding with boost::throw_exception
template<class E>
void
BOOST_NORETURN
throw_exception(E e)
{
    throw e;
}
#endif

void
throw_bad_alloc(
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        std::bad_alloc()
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

void
throw_length_error(
    char const* what,
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        std::length_error(what)
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

void
throw_invalid_argument(
    char const* what,
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        std::invalid_argument(what)
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

void
throw_out_of_range(
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        std::out_of_range(
            "out of range")
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

void
throw_system_error(
    error_code const& ec,
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        system_error(ec)
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

void
throw_system_error(
    error e,
    source_location const& loc)
{
    (void)loc;
    throw_exception(
        system_error(e)
#if ! defined(BOOST_JSON_STANDALONE)
        , loc
#endif
        );
}

} // detail
BOOST_JSON_NS_END

#endif
