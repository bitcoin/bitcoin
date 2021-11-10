//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines c++14 index_sequence implementation
// ***************************************************************************

#ifndef BOOST_TEST_DATA_INDEX_SEQUENCE_HPP
#define BOOST_TEST_DATA_INDEX_SEQUENCE_HPP

// Boost.Test
#include <boost/test/data/config.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

// ************************************************************************** //
// **************             data::index_sequence             ************** //
// ************************************************************************** //

template <std::size_t... Ns>
struct index_sequence {};

template<typename IS1, typename IS2>
struct merge_index_sequence;

template <std::size_t... Ns1, std::size_t... Ns2>
struct merge_index_sequence<index_sequence<Ns1...>, index_sequence<Ns2...>> {
    typedef index_sequence<Ns1..., Ns2...> type;
};

template <std::size_t B, std::size_t E, typename Enabler = void>
struct make_index_sequence {
    typedef typename merge_index_sequence<typename make_index_sequence<B,(B+E)/2>::type,
                                          typename make_index_sequence<(B+E)/2,E>::type>::type type;
};

template <std::size_t B, std::size_t E>
struct make_index_sequence<B,E,typename std::enable_if<E==B+1,void>::type> {
    typedef index_sequence<B> type;
};

template <>
struct make_index_sequence<0, 0> {
    typedef index_sequence<> type;
};

template <typename... T>
using index_sequence_for = typename make_index_sequence<0, sizeof...(T)>::type;

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_INDEX_SEQUENCE_HPP

