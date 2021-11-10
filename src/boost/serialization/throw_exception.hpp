#ifndef BOOST_SERIALIZATION_THROW_EXCEPTION_HPP_INCLUDED
#define BOOST_SERIALIZATION_THROW_EXCEPTION_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif

//  boost/throw_exception.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

namespace boost {
namespace serialization {

#ifdef BOOST_NO_EXCEPTIONS

BOOST_NORETURN inline void throw_exception(std::exception const & e) {
    ::boost::throw_exception(e);
}

#else

template<class E> BOOST_NORETURN inline void throw_exception(E const & e){
    throw e;
}

#endif

} // namespace serialization
} // namespace boost

#endif // #ifndef BOOST_SERIALIZATION_THROW_EXCEPTION_HPP_INCLUDED
