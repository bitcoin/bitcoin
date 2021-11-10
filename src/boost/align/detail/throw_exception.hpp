/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_DETAIL_THROW_EXCEPTION_HPP
#define BOOST_ALIGN_DETAIL_THROW_EXCEPTION_HPP

#include <boost/config.hpp>
#if defined(BOOST_NO_EXCEPTIONS)
#include <exception>
#endif

namespace boost {

#if defined(BOOST_NO_EXCEPTIONS)
BOOST_NORETURN void throw_exception(const std::exception&);
#endif

namespace alignment {
namespace detail {

#if !defined(BOOST_NO_EXCEPTIONS)
template<class E>
BOOST_NORETURN inline void
throw_exception(const E& error)
{
    throw error;
}
#else
BOOST_NORETURN inline void
throw_exception(const std::exception& error)
{
    boost::throw_exception(error);
}
#endif

} /* detail */
} /* alignment */
} /* boost */

#endif
