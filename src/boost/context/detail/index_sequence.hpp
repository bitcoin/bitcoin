
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
#define BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>

#if defined(BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE)
#include <boost/mp11/integer_sequence.hpp>
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

#if ! defined(BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE)
template< std::size_t ... I >
using index_sequence = std::index_sequence< I ... >;
template< std::size_t I >
using make_index_sequence = std::make_index_sequence< I >;
template< typename ... T >
using index_sequence_for = std::index_sequence_for< T ... >;
#else
template< std::size_t ... I >
using index_sequence = mp11::index_sequence< I ... >;
template< std::size_t I >
using make_index_sequence = mp11::make_index_sequence< I >;
template< typename ... T >
using index_sequence_for = mp11::index_sequence_for< T ... >;
#endif

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
