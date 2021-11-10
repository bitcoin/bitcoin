/*
Copyright Barrett Adair 2016-2017

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_CLBL_TRTS_DETAIL_POLYFILLS_MAKE_INDEX_SEQUENCE_HPP
#define BOOST_CLBL_TRTS_DETAIL_POLYFILLS_MAKE_INDEX_SEQUENCE_HPP

#undef BOOST_CLBL_TRTS_IX_SEQ
#define BOOST_CLBL_TRTS_IX_SEQ(...) \
    ::boost::callable_traits::detail::index_sequence<__VA_ARGS__>

#undef BOOST_CLBL_TRTS_MAKE_IX_SEQ
#define BOOST_CLBL_TRTS_MAKE_IX_SEQ(...) \
    ::boost::callable_traits::detail::make_index_sequence<__VA_ARGS__>

namespace boost { namespace callable_traits { namespace detail { 

template<std::size_t...>
struct index_sequence { using type = index_sequence; };

template<typename, typename>
struct concat;

template<std::size_t... I1, std::size_t... I2>
struct concat<index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1)+I2)...> {};

template<std::size_t N>
    struct make_index_sequence_t;

template<std::size_t N>
struct make_index_sequence_t : concat< 
    typename make_index_sequence_t<N/2>::type,
    typename make_index_sequence_t<N - N/2>::type >::type {};

template<>
struct make_index_sequence_t<0> : index_sequence<> {};

template<>
struct make_index_sequence_t<1> : index_sequence<0> {};

template<std::size_t... I>
using make_index_sequence = typename make_index_sequence_t<I...>::type;

}}} // namespace boost::callable_traits::detail

#endif // #ifndef BOOST_CLBL_TRTS_DETAIL_POLYFILLS_MAKE_INDEX_SEQUENCE_HPP
