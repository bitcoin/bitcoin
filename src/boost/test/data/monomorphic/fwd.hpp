//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Forward declares monomorphic datasets interfaces
// ***************************************************************************

#ifndef BOOST_TEST_DATA_MONOMORPHIC_FWD_HPP_102212GER
#define BOOST_TEST_DATA_MONOMORPHIC_FWD_HPP_102212GER

// Boost.Test
#include <boost/test/data/config.hpp>
#include <boost/test/data/size.hpp>

#include <boost/test/utils/is_forward_iterable.hpp>

// Boost
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/mpl/bool.hpp>

// STL
#include <tuple>

#include <boost/test/detail/suppress_warnings.hpp>

// STL
#include <initializer_list>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace data {

namespace monomorphic {


#if !defined(BOOST_TEST_DOXYGEN_DOC__)

template<typename T>
class singleton;

template<typename C>
class collection;

template<typename T>
class array;

template<typename T>
class init_list;

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_HDR_TUPLE)
template<class dataset_t, class ...Args>
class delayed_dataset;
#endif

#endif

// ************************************************************************** //
// **************            monomorphic::is_dataset           ************** //
// ************************************************************************** //

//! Helper metafunction indicating if the specified type is a dataset.
template<typename DataSet>
struct is_dataset : mpl::false_ {};

//____________________________________________________________________________//

//! A reference to a dataset is a dataset
template<typename DataSet>
struct is_dataset<DataSet&> : is_dataset<DataSet> {};
template<typename DataSet>
struct is_dataset<DataSet&&> : is_dataset<DataSet> {};

//____________________________________________________________________________//

//! A const dataset is a dataset
template<typename DataSet>
struct is_dataset<DataSet const> : is_dataset<DataSet> {};

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

//! Helper to check if a list of types contains a dataset
template<class DataSet, class...>
struct has_dataset : is_dataset<DataSet> {};

template<class DataSet0, class DataSet1, class... DataSetTT>
struct has_dataset<DataSet0, DataSet1, DataSetTT...>
  : std::integral_constant<bool, is_dataset<DataSet0>::value || has_dataset<DataSet1, DataSetTT...>::value>
{};
#endif

} // namespace monomorphic

// ************************************************************************** //
// **************                  data::make                  ************** //
// ************************************************************************** //

//! @brief Creates a dataset from a value, a collection or an array
//!
//! This function has several overloads:
//! @code
//! // returns ds if ds is already a dataset
//! template <typename DataSet> DataSet make(DataSet&& ds); 
//!
//! // creates a singleton dataset, for non forward iterable and non dataset type T
//! // (a C string is not considered as a sequence).
//! template <typename T> monomorphic::singleton<T> make(T&& v); 
//! monomorphic::singleton<char*> make( char* str );
//! monomorphic::singleton<char const*> make( char const* str );
//!
//! // creates a collection dataset, for forward iterable and non dataset type C
//! template <typename C> monomorphic::collection<C> make(C && c);
//!
//! // creates an array dataset
//! template<typename T, std::size_t size> monomorphic::array<T> make( T (&a)[size] );
//! @endcode
template<typename DataSet>
inline typename std::enable_if<monomorphic::is_dataset<DataSet>::value,DataSet>::type
make(DataSet&& ds)
{
    return std::forward<DataSet>( ds );
}

//____________________________________________________________________________//

// warning: doxygen is apparently unable to handle @overload from different files, so if the overloads
// below are not declared with @overload in THIS file, they do not appear in the documentation.

//! @overload boost::unit_test::data::make()
template<typename T>
inline typename std::enable_if<!is_container_forward_iterable<T>::value && 
                               !monomorphic::is_dataset<T>::value &&
                               !is_array<typename remove_reference<T>::type>::value, 
                               monomorphic::singleton<T>>::type
make( T&& v );

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename C>
inline typename std::enable_if<is_container_forward_iterable<C>::value,monomorphic::collection<C>>::type
make( C&& c );

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename T, std::size_t size>
inline monomorphic::array< typename boost::remove_const<T>::type >
make( T (&a)[size] );

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
inline monomorphic::singleton<char*>
make( char* str );

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
inline monomorphic::singleton<char const*>
make( char const* str );

//____________________________________________________________________________//

//! @overload boost::unit_test::data::make()
template<typename T>
inline monomorphic::init_list<T>
make( std::initializer_list<T>&& );

//____________________________________________________________________________//

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_TEST_ERRONEOUS_INIT_LIST)
//! @overload boost::unit_test::data::make()
template<class T, class ...Args>
inline typename std::enable_if<
  !monomorphic::has_dataset<T, Args...>::value,
  monomorphic::init_list<T>
>::type
make( T&& arg0, Args&&... args );
#endif

//____________________________________________________________________________//

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    !defined(BOOST_NO_CXX11_HDR_TUPLE)
template<class dataset_t, class ...Args>
inline typename std::enable_if<
  monomorphic::is_dataset< dataset_t >::value,
  monomorphic::delayed_dataset<dataset_t, Args...>
>::type
make_delayed(Args... args);
#endif

//____________________________________________________________________________//

namespace result_of {

//! Result of the make call.
template<typename DataSet>
struct make {
    typedef decltype( data::make( declval<DataSet>() ) ) type;
};

} // namespace result_of

} // namespace data
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_DATA_MONOMORPHIC_FWD_HPP_102212GER

