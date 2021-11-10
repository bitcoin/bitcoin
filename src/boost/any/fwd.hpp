// Copyright Antony Polukhin, 2021.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed by Ruslan Arutyunyan
#ifndef BOOST_ANY_ANYS_FWD_HPP
#define BOOST_ANY_ANYS_FWD_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

 #include <boost/type_traits/alignment_of.hpp>

namespace boost {

class any;

namespace anys {

template<std::size_t OptimizeForSize = sizeof(void*), std::size_t OptimizeForAlignment = boost::alignment_of<void*>::value>
class basic_any;

namespace detail {
    template <class T>
    struct is_basic_any: public false_type {};


    template<std::size_t OptimizeForSize, std::size_t OptimizeForAlignment>
    struct is_basic_any<boost::anys::basic_any<OptimizeForSize, OptimizeForAlignment> > : public true_type {};
} // namespace detail

} // namespace anys

} // namespace boost

#endif  // #ifndef BOOST_ANY_ANYS_FWD_HPP
