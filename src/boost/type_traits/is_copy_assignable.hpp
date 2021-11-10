//  (C) Copyright Ion Gaztanaga 2014.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_COPY_ASSIGNABLE_HPP_INCLUDED
#define BOOST_TT_IS_COPY_ASSIGNABLE_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/type_traits/is_noncopyable.hpp>

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS) && !defined(BOOST_NO_CXX11_DECLTYPE) \
   && !defined(BOOST_INTEL_CXX_VERSION) && \
      !(defined(BOOST_MSVC) && _MSC_VER == 1800)
#define BOOST_TT_CXX11_IS_COPY_ASSIGNABLE
#include <boost/type_traits/declval.hpp>
#else
   //For compilers without decltype
   #include <boost/type_traits/is_const.hpp>
   #include <boost/type_traits/is_array.hpp>
   #include <boost/type_traits/add_reference.hpp>
   #include <boost/type_traits/remove_reference.hpp>
#endif

namespace boost {

namespace detail{

template <bool DerivedFromNoncopyable, class T>
struct is_copy_assignable_impl2 {

// Intel compiler has problems with SFINAE for copy constructors and deleted functions:
//
// error: function *function_name* cannot be referenced -- it is a deleted function
// static boost::type_traits::yes_type test(T1&, decltype(T1(boost::declval<T1&>()))* = 0);
//                                                        ^ 
//
// MSVC 12.0 (Visual 2013) has problems when the copy constructor has been deleted. See:
// https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
#if defined(BOOST_TT_CXX11_IS_COPY_ASSIGNABLE)
    typedef boost::type_traits::yes_type yes_type;
    typedef boost::type_traits::no_type  no_type;

    template <class U>
    static decltype(::boost::declval<U&>() = ::boost::declval<const U&>(), yes_type() ) test(int);

    template <class>
    static no_type test(...);

    static const bool value = sizeof(test<T>(0)) == sizeof(yes_type);

#else
    static BOOST_DEDUCED_TYPENAME boost::add_reference<T>::type produce();

    template <class T1>
    static boost::type_traits::no_type test(T1&, typename T1::boost_move_no_copy_constructor_or_assign* = 0);

    static boost::type_traits::yes_type test(...);
    // If you see errors like this:
    //
    //      `'T::operator=(const T&)' is private`
    //      `boost/type_traits/is_copy_assignable.hpp:NN:M: error: within this context`
    //
    // then you are trying to call that macro for a structure defined like that:
    //
    //      struct T {
    //          ...
    //      private:
    //          T & operator=(const T &);
    //          ...
    //      };
    //
    // To fix that you must modify your structure:
    //
    //      // C++03 and C++11 version
    //      struct T: private boost::noncopyable {
    //          ...
    //      private:
    //          T & operator=(const T &);
    //          ...
    //      };
    //
    //      // C++11 version
    //      struct T {
    //          ...
    //      private:
    //          T& operator=(const T &) = delete;
    //          ...
    //      };
    BOOST_STATIC_CONSTANT(bool, value = (
            sizeof(test(produce())) == sizeof(boost::type_traits::yes_type)
    ));
   #endif
};

template <class T>
struct is_copy_assignable_impl2<true, T> {
    BOOST_STATIC_CONSTANT(bool, value = false);
};

template <class T>
struct is_copy_assignable_impl {

#if !defined(BOOST_TT_CXX11_IS_COPY_ASSIGNABLE)
    //For compilers without decltype, at least return false on const types, arrays
    //types derived from boost::noncopyable and types defined as BOOST_MOVEABLE_BUT_NOT_COPYABLE
    typedef BOOST_DEDUCED_TYPENAME boost::remove_reference<T>::type unreferenced_t;
    BOOST_STATIC_CONSTANT(bool, value = (
        boost::detail::is_copy_assignable_impl2<
            boost::is_noncopyable<T>::value
            || boost::is_const<unreferenced_t>::value || boost::is_array<unreferenced_t>::value
            ,T
        >::value
    ));
    #else
    BOOST_STATIC_CONSTANT(bool, value = (
        boost::detail::is_copy_assignable_impl2<
            boost::is_noncopyable<T>::value,T
        >::value
    ));
    #endif
};

} // namespace detail

template <class T> struct is_copy_assignable : public integral_constant<bool, ::boost::detail::is_copy_assignable_impl<T>::value>{};
template <> struct is_copy_assignable<void> : public false_type{};
#ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template <> struct is_copy_assignable<void const> : public false_type{};
template <> struct is_copy_assignable<void const volatile> : public false_type{};
template <> struct is_copy_assignable<void volatile> : public false_type{};
#endif

} // namespace boost

#endif // BOOST_TT_IS_COPY_ASSIGNABLE_HPP_INCLUDED
