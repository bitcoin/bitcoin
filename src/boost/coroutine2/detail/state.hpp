
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_ASYMMETRIC_COROUTINE_HPP
#define BOOST_COROUTINES2_DETAIL_ASYMMETRIC_COROUTINE_HPP

#include <exception>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/coroutine2/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

enum class state_t : unsigned int {
    none       = 0,
    complete   = 1 << 1,
    unwind     = 1 << 2,
    destroy    = 1 << 3
};


inline
constexpr state_t
operator&( state_t l, state_t r) {
    return static_cast< state_t >(
            static_cast< unsigned int >( l) & static_cast< unsigned int >( r) );
}

inline
constexpr state_t
operator|( state_t l, state_t r) {
    return static_cast< state_t >(
            static_cast< unsigned int >( l) | static_cast< unsigned int >( r) );
}

inline
constexpr state_t
operator^( state_t l, state_t r) {
    return static_cast< state_t >(
            static_cast< unsigned int >( l) ^ static_cast< unsigned int >( r) );
}

inline
constexpr state_t
operator~( state_t l) {
    return static_cast< state_t >( ~static_cast< unsigned int >( l) );
}

inline
state_t &
operator&=( state_t & l, state_t r) {
    l = l & r;
    return l;
}

inline
state_t &
operator|=( state_t & l, state_t r) {
    l = l | r;
    return l;
}

inline
state_t &
operator^=( state_t & l, state_t r) {
    l = l ^ r;
    return l;
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_ASYMMETRIC_COROUTINE_HPP
