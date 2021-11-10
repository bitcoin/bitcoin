
#ifndef BOOST_CONTRACT_DETAIL_EXCEPTION_HPP_
#define BOOST_CONTRACT_DETAIL_EXCEPTION_HPP_

// Copyright (C) 2008-2019 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/config.hpp>
#include <exception>

namespace boost { namespace contract { namespace detail {

// Using this instead of std::uncaught_exception() because
// std::uncaught_exception will be removed in C++20.
inline bool uncaught_exception() BOOST_NOEXCEPT {
    // Alternatively, this could just return `boost::core::uncaught_exceptions()
    // > 0` but that emulates the exception count which is not needed by this
    // lib (the implementation below is simpler and could be faster).
    #ifdef __cpp_lib_uncaught_exceptions
        return std::uncaught_exceptions() > 0;
    #else
        return std::uncaught_exception();
    #endif
}

} } } // namespace

#endif // #include guard

