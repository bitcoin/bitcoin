//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Defines helper routines and types for merging monomorphic samples
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_SAMPLE_MERGE_HPP
#define BOOST_TEST_DATA_MONOMORPHIC_SAMPLE_MERGE_HPP

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/index_sequence.hpp>

#include <boost/test/data/monomorphic/fwd.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

namespace boost {
namespace unit_test {
namespace data {
namespace monomorphic {

//____________________________________________________________________________//

namespace ds_detail {

template <class T>
struct is_tuple : std::false_type {};

template <class ...T>
struct is_tuple<std::tuple<T...>> : std::true_type {};

template <class T>
struct is_tuple<T&&> : is_tuple<typename std::decay<T>::type> {};

template <class T>
struct is_tuple<T&> : is_tuple<typename std::decay<T>::type> {};

template<typename T>
inline auto as_tuple_impl_xvalues( T const & arg, std::false_type  /* is_rvalue_ref */ )
  -> decltype(std::tuple<T const&>(arg)) {
    //return std::tuple<T const&>(arg);
    return std::forward_as_tuple(arg);
}

template<typename T>
inline auto as_tuple_impl_xvalues( T && arg, std::true_type  /* is_rvalue_ref */ )
  -> decltype(std::make_tuple(std::forward<T>(arg))) {
    return std::make_tuple(std::forward<T>(arg));
}


template<typename T>
inline auto as_tuple_impl( T && arg, std::false_type  /* is_tuple = nullptr */ )
  -> decltype(as_tuple_impl_xvalues(std::forward<T>(arg),
              typename std::is_rvalue_reference<T&&>::type())) {
    return as_tuple_impl_xvalues(std::forward<T>(arg),
                                 typename std::is_rvalue_reference<T&&>::type());
}

//____________________________________________________________________________//

template<typename T>
inline T &&
as_tuple_impl(T && arg, std::true_type  /* is_tuple */ ) {
    return std::forward<T>(arg);
}

template<typename T>
inline auto as_tuple( T && arg )
  -> decltype( as_tuple_impl(std::forward<T>(arg),
                             typename ds_detail::is_tuple<T>::type()) ) {
  return as_tuple_impl(std::forward<T>(arg),
                       typename ds_detail::is_tuple<T>::type());
}

//____________________________________________________________________________//

} // namespace ds_detail

template<typename T1, typename T2>
inline auto
sample_merge( T1 && a1, T2 && a2 )
    -> decltype( std::tuple_cat(ds_detail::as_tuple(std::forward<T1>(a1)),
                                ds_detail::as_tuple(std::forward<T2>(a2)) ) ) {
    return std::tuple_cat(ds_detail::as_tuple(std::forward<T1>(a1)),
                          ds_detail::as_tuple(std::forward<T2>(a2)));
}

} // namespace monomorphic
} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_SAMPLE_MERGE_HPP

