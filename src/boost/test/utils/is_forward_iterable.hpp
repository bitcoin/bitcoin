//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//! @file
//! Defines the is_forward_iterable collection type trait
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_IS_FORWARD_ITERABLE_HPP
#define BOOST_TEST_UTILS_IS_FORWARD_ITERABLE_HPP

#if defined(BOOST_NO_CXX11_DECLTYPE) || \
    defined(BOOST_NO_CXX11_NULLPTR) || \
    defined(BOOST_NO_CXX11_TRAILING_RESULT_TYPES)

  // this feature works with VC2012 upd 5 while BOOST_NO_CXX11_TRAILING_RESULT_TYPES is defined
  #if !defined(BOOST_MSVC) || BOOST_MSVC_FULL_VER < 170061232 /* VC2012 upd 5 */
    #define BOOST_TEST_FWD_ITERABLE_CXX03
  #endif
#endif

#if defined(BOOST_TEST_FWD_ITERABLE_CXX03)
// Boost
#include <boost/mpl/bool.hpp>

// STL
#include <list>
#include <vector>
#include <map>
#include <set>

#else

// Boost
#include <boost/static_assert.hpp>
#include <boost/utility/declval.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/test/utils/is_cstring.hpp>

// STL
#include <utility>
#include <type_traits>

#endif
//____________________________________________________________________________//

namespace boost {
namespace unit_test {

template<typename T>
struct is_forward_iterable;

// ************************************************************************** //
// **************             is_forward_iterable              ************** //
// ************************************************************************** //

#if defined(BOOST_TEST_FWD_ITERABLE_CXX03) && !defined(BOOST_TEST_DOXYGEN_DOC__)
template<typename T>
struct is_forward_iterable : public mpl::false_ {};

template<typename T>
struct is_forward_iterable<T const> : public is_forward_iterable<T> {};

template<typename T>
struct is_forward_iterable<T&> : public is_forward_iterable<T> {};

template<typename T, std::size_t N>
struct is_forward_iterable< T [N] > : public mpl::true_ {};

template<typename T, typename A>
struct is_forward_iterable< std::vector<T, A> > : public mpl::true_ {};

template<typename T, typename A>
struct is_forward_iterable< std::list<T, A> > : public mpl::true_ {};

template<typename K, typename V, typename C, typename A>
struct is_forward_iterable< std::map<K, V, C, A> > : public mpl::true_ {};

template<typename K, typename C, typename A>
struct is_forward_iterable< std::set<K, C, A> > : public mpl::true_ {};

// string is also forward iterable, even if sometimes we want to treat the
// assertions differently.
template<>
struct is_forward_iterable< std::string > : public mpl::true_ {};

#else

namespace ut_detail {

// SFINAE helper
template<typename T>
struct is_present : public mpl::true_ {};

//____________________________________________________________________________//

// some compiler do not implement properly decltype non expression involving members (eg. VS2013)
// a workaround is to use -> decltype syntax.
template <class T>
struct has_member_size {
private:
    struct nil_t {};
    template<typename U> static auto  test( U* ) -> decltype(boost::declval<U>().size());
    template<typename>   static nil_t test( ... );

public:
    static bool const value = !std::is_same< decltype(test<T>( nullptr )), nil_t>::value;
};

//____________________________________________________________________________//

template <class T>
struct has_member_begin {
private:
    struct nil_t {};
    template<typename U>  static auto  test( U* ) -> decltype(std::begin(boost::declval<U&>())); // does not work with boost::begin
    template<typename>    static nil_t test( ... );
public:
    static bool const value = !std::is_same< decltype(test<T>( nullptr )), nil_t>::value;
};

//____________________________________________________________________________//

template <class T>
struct has_member_end {
private:
    struct nil_t {};
    template<typename U>  static auto  test( U* ) -> decltype(std::end(boost::declval<U&>())); // does not work with boost::end
    template<typename>    static nil_t test( ... );
public:
    static bool const value = !std::is_same< decltype(test<T>( nullptr )), nil_t>::value;
};

//____________________________________________________________________________//

template <class T, class enabled = void>
struct is_forward_iterable_impl : std::false_type {
};

template <class T>
struct is_forward_iterable_impl<
    T,
    typename std::enable_if<
      has_member_begin<T>::value &&
      has_member_end<T>::value
    >::type
> : std::true_type
{};

//____________________________________________________________________________//

template <class T, class enabled = void>
struct is_container_forward_iterable_impl : std::false_type {
};

template <class T>
struct is_container_forward_iterable_impl<
    T,
    typename std::enable_if<
      is_present<typename T::const_iterator>::value &&
      is_present<typename T::value_type>::value &&
      has_member_size<T>::value &&
      is_forward_iterable_impl<T>::value
    >::type
> : is_forward_iterable_impl<T>
{};

//____________________________________________________________________________//

} // namespace ut_detail

/*! Indicates that a specific type implements the forward iterable concept. */
template<typename T>
struct is_forward_iterable {
    typedef typename std::remove_reference<T>::type T_ref;
    typedef ut_detail::is_forward_iterable_impl<T_ref> is_fwd_it_t;
    typedef mpl::bool_<is_fwd_it_t::value> type;
    enum { value = is_fwd_it_t::value };
};

/*! Indicates that a specific type implements the forward iterable concept. */
template<typename T>
struct is_container_forward_iterable {
    typedef typename std::remove_reference<T>::type T_ref;
    typedef ut_detail::is_container_forward_iterable_impl<T_ref> is_fwd_it_t;
    typedef mpl::bool_<is_fwd_it_t::value> type;
    enum { value = is_fwd_it_t::value };
};

#endif /* defined(BOOST_TEST_FWD_ITERABLE_CXX03) */


//! Helper structure for accessing the content of a container or an array
template <typename T, bool is_forward_iterable = is_forward_iterable<T>::value >
struct bt_iterator_traits;

template <typename T>
struct bt_iterator_traits< T, true >{
    BOOST_STATIC_ASSERT((is_forward_iterable<T>::value));

#if defined(BOOST_TEST_FWD_ITERABLE_CXX03) || \
    (defined(BOOST_MSVC) && (BOOST_MSVC_FULL_VER <= 170061232))
    typedef typename T::const_iterator const_iterator;
    typedef typename std::iterator_traits<const_iterator>::value_type value_type;
#else
    typedef decltype(boost::declval<
        typename boost::add_const<
          typename boost::remove_reference<T>::type
        >::type>().begin()) const_iterator;

    typedef typename std::iterator_traits<const_iterator>::value_type value_type;
#endif /* BOOST_TEST_FWD_ITERABLE_CXX03 */

    static const_iterator begin(T const& container) {
        return container.begin();
    }
    static const_iterator end(T const& container) {
        return container.end();
    }

#if defined(BOOST_TEST_FWD_ITERABLE_CXX03) || \
    (defined(BOOST_MSVC) && (BOOST_MSVC_FULL_VER <= 170061232))
    static std::size_t
    size(T const& container) {
        return container.size();
    }
#else
    static std::size_t
    size(T const& container) {
        return size(container,
                    std::integral_constant<bool, ut_detail::has_member_size<T>::value>());
    }
private:
    static std::size_t
    size(T const& container, std::true_type)  { return container.size(); }

    static std::size_t
    size(T const& container, std::false_type) { return std::distance(begin(container), end(container)); }
#endif /* BOOST_TEST_FWD_ITERABLE_CXX03 */
};

template <typename T, std::size_t N>
struct bt_iterator_traits< T [N], true > {
    typedef typename boost::add_const<T>::type T_const;
    typedef typename boost::add_pointer<T_const>::type const_iterator;
    typedef T value_type;

    static const_iterator begin(T_const (&array)[N]) {
        return &array[0];
    }
    static const_iterator end(T_const (&array)[N]) {
        return &array[N];
    }
    static std::size_t size(T_const (&)[N]) {
        return N;
    }
};

} // namespace unit_test
} // namespace boost

#endif // BOOST_TEST_UTILS_IS_FORWARD_ITERABLE_HPP
