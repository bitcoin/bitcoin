//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! Defines the is_cstring type trait
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_IS_CSTRING_HPP
#define BOOST_TEST_UTILS_IS_CSTRING_HPP

// Boost
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/add_const.hpp>

#include <boost/test/utils/basic_cstring/basic_cstring_fwd.hpp>
#include <string>

#if defined(BOOST_TEST_STRING_VIEW)
#include <string_view>
#endif

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************                  is_cstring                  ************** //
// ************************************************************************** //

namespace ut_detail {

template<typename T>
struct is_cstring_impl : public mpl::false_ {};

template<typename T>
struct is_cstring_impl<T const*> : public is_cstring_impl<T*> {};

template<typename T>
struct is_cstring_impl<T const* const> : public is_cstring_impl<T*> {};

template<>
struct is_cstring_impl<char*> : public mpl::true_ {};

template<>
struct is_cstring_impl<wchar_t*> : public mpl::true_ {};

template <typename T, bool is_cstring = is_cstring_impl<typename boost::decay<T>::type>::value >
struct deduce_cstring_transform_impl;

template <typename T, bool is_cstring >
struct deduce_cstring_transform_impl<T&, is_cstring> : public deduce_cstring_transform_impl<T, is_cstring>{};

template <typename T, bool is_cstring >
struct deduce_cstring_transform_impl<T const, is_cstring> : public deduce_cstring_transform_impl<T, is_cstring>{};

template <typename T>
struct deduce_cstring_transform_impl<T, true> {
    typedef typename boost::add_const<
        typename boost::remove_pointer<
            typename boost::decay<T>::type
        >::type
    >::type U;
    typedef boost::unit_test::basic_cstring<U> type;
};

template <typename T>
struct deduce_cstring_transform_impl< T, false > {
    typedef typename
        boost::remove_const<
            typename boost::remove_reference<T>::type
        >::type type;
};

template <typename T>
struct deduce_cstring_transform_impl< std::basic_string<T, std::char_traits<T> >, false > {
    typedef boost::unit_test::basic_cstring<typename boost::add_const<T>::type> type;
};

#if defined(BOOST_TEST_STRING_VIEW)
template <typename T>
struct deduce_cstring_transform_impl< std::basic_string_view<T, std::char_traits<T> >, false > {
private:
    using sv_t = std::basic_string_view<T, std::char_traits<T> > ;
  
public:
    using type = stringview_cstring_helper<typename boost::add_const<T>::type, sv_t>;
};
#endif

} // namespace ut_detail

template<typename T>
struct is_cstring : public ut_detail::is_cstring_impl<typename decay<T>::type> {};

template<typename T, bool is_cstring = is_cstring<typename boost::decay<T>::type>::value >
struct is_cstring_comparable: public mpl::false_ {};

template<typename T>
struct is_cstring_comparable< T, true > : public mpl::true_ {};

template<typename T>
struct is_cstring_comparable< std::basic_string<T, std::char_traits<T> >, false > : public mpl::true_ {};

#if defined(BOOST_TEST_STRING_VIEW)
template<typename T>
struct is_cstring_comparable< std::basic_string_view<T, std::char_traits<T> >, false > : public mpl::true_ {};
#endif

template<typename T>
struct is_cstring_comparable< boost::unit_test::basic_cstring<T>, false > : public mpl::true_ {};

template <class T>
struct deduce_cstring_transform {
    typedef typename
        boost::remove_const<
            typename boost::remove_reference<T>::type
        >::type U;
    typedef typename ut_detail::deduce_cstring_transform_impl<typename boost::decay<U>::type>::type type;
};

} // namespace unit_test
} // namespace boost

#endif // BOOST_TEST_UTILS_IS_CSTRING_HPP
