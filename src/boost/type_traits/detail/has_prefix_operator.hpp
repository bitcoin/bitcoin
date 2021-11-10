//  (C) Copyright 2009-2011 Frederic Bron, Robert Stewart, Steven Watanabe & Roman Perepelitsa.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#include <boost/config.hpp>
#include <boost/type_traits/detail/config.hpp>

#if defined(BOOST_TT_HAS_ACCURATE_BINARY_OPERATOR_DETECTION)

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/make_void.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <utility>

#ifdef BOOST_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif
#if defined(BOOST_MSVC)
#   pragma warning ( push )
#   pragma warning ( disable : 4804)
#endif

namespace boost
{

   namespace binary_op_detail {

      struct dont_care;

      template <class T, class Ret, class = void>
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _ret_imp) : public boost::false_type {};

      template <class T, class Ret>
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _ret_imp)<T, Ret, typename boost::make_void<decltype(BOOST_TT_TRAIT_OP std::declval<typename add_reference<T>::type>()) >::type>
         : public boost::integral_constant<bool, ::boost::is_convertible<decltype(BOOST_TT_TRAIT_OP std::declval<typename add_reference<T>::type>() ), Ret>::value> {};

      template <class T, class = void >
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _void_imp) : public boost::false_type {};

      template <class T>
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _void_imp)<T, typename boost::make_void<decltype(BOOST_TT_TRAIT_OP std::declval<typename add_reference<T>::type>())>::type>
         : public boost::integral_constant<bool, ::boost::is_void<decltype(BOOST_TT_TRAIT_OP std::declval<typename add_reference<T>::type>())>::value> {};

      template <class T, class = void>
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _dc_imp) : public boost::false_type {};

      template <class T>
      struct BOOST_JOIN(BOOST_TT_TRAIT_NAME, _dc_imp)<T, typename boost::make_void<decltype(BOOST_TT_TRAIT_OP std::declval<typename add_reference<T>::type>() )>::type>
         : public boost::true_type {};

   }

   template <class T, class Ret = boost::binary_op_detail::dont_care>
   struct BOOST_TT_TRAIT_NAME : public boost::binary_op_detail::BOOST_JOIN(BOOST_TT_TRAIT_NAME, _ret_imp) <T, Ret> {};
   template <class T>
   struct BOOST_TT_TRAIT_NAME<T, void> : public boost::binary_op_detail::BOOST_JOIN(BOOST_TT_TRAIT_NAME, _void_imp) <T> {};
   template <class T>
   struct BOOST_TT_TRAIT_NAME<T, boost::binary_op_detail::dont_care> : public boost::binary_op_detail::BOOST_JOIN(BOOST_TT_TRAIT_NAME, _dc_imp) <T> {};


}

#ifdef BOOST_GCC
#pragma GCC diagnostic pop
#endif
#if defined(BOOST_MSVC)
#   pragma warning ( pop )
#endif

#else

#include <boost/type_traits/detail/yes_no_type.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/remove_reference.hpp>

// cannot include this header without getting warnings of the kind:
// gcc:
//    warning: value computed is not used
//    warning: comparison between signed and unsigned integer expressions
// msvc:
//    warning C4146: unary minus operator applied to unsigned type, result still unsigned
//    warning C4804: '-' : unsafe use of type 'bool' in operation
// cannot find another implementation -> declared as system header to suppress these warnings.
#if defined(__GNUC__)
#   pragma GCC system_header
#elif defined(BOOST_MSVC)
#   pragma warning ( push )
#   pragma warning ( disable : 4146 4804 4913 4244 4800)
#   if BOOST_WORKAROUND(BOOST_MSVC_FULL_VER, >= 140050000)
#       pragma warning ( disable : 6334)
#   endif
#   if BOOST_WORKAROUND(_MSC_VER, >= 1913)
#       pragma warning ( disable : 4834)
#   endif
#endif



namespace boost {
namespace detail {

// This namespace ensures that argument-dependent name lookup does not mess things up.
namespace BOOST_JOIN(BOOST_TT_TRAIT_NAME,_impl) {

// 1. a function to have an instance of type T without requiring T to be default
// constructible
template <typename T> T &make();


// 2. we provide our operator definition for types that do not have one already

// a type returned from operator BOOST_TT_TRAIT_OP when no such operator is
// found in the type's own namespace (our own operator is used) so that we have
// a means to know that our operator was used
struct no_operator { };

// this class allows implicit conversions and makes the following operator
// definition less-preferred than any other such operators that might be found
// via argument-dependent name lookup
struct any { template <class T> any(T const&); };

// when operator BOOST_TT_TRAIT_OP is not available, this one is used
no_operator operator BOOST_TT_TRAIT_OP (const any&);


// 3. checks if the operator returns void or not
// conditions: Rhs!=void

// we first redefine "operator," so that we have no compilation error if
// operator BOOST_TT_TRAIT_OP returns void and we can use the return type of
// (BOOST_TT_TRAIT_OP rhs, returns_void_t()) to deduce if
// operator BOOST_TT_TRAIT_OP returns void or not:
// - operator BOOST_TT_TRAIT_OP returns void   -> (BOOST_TT_TRAIT_OP rhs, returns_void_t()) returns returns_void_t
// - operator BOOST_TT_TRAIT_OP returns !=void -> (BOOST_TT_TRAIT_OP rhs, returns_void_t()) returns int
struct returns_void_t { };
template <typename T> int operator,(const T&, returns_void_t);
template <typename T> int operator,(const volatile T&, returns_void_t);

// this intermediate trait has member value of type bool:
// - value==true -> operator BOOST_TT_TRAIT_OP returns void
// - value==false -> operator BOOST_TT_TRAIT_OP does not return void
template < typename Rhs >
struct operator_returns_void {
   // overloads of function returns_void make the difference
   // yes_type and no_type have different size by construction
   static ::boost::type_traits::yes_type returns_void(returns_void_t);
   static ::boost::type_traits::no_type returns_void(int);
   BOOST_STATIC_CONSTANT(bool, value = (sizeof(::boost::type_traits::yes_type)==sizeof(returns_void((BOOST_TT_TRAIT_OP make<Rhs>(),returns_void_t())))));
};


// 4. checks if the return type is Ret or Ret==dont_care
// conditions: Rhs!=void

struct dont_care { };

template < typename Rhs, typename Ret, bool Returns_void >
struct operator_returns_Ret;

template < typename Rhs >
struct operator_returns_Ret < Rhs, dont_care, true > {
   BOOST_STATIC_CONSTANT(bool, value = true);
};

template < typename Rhs >
struct operator_returns_Ret < Rhs, dont_care, false > {
   BOOST_STATIC_CONSTANT(bool, value = true);
};

template < typename Rhs >
struct operator_returns_Ret < Rhs, void, true > {
   BOOST_STATIC_CONSTANT(bool, value = true);
};

template < typename Rhs >
struct operator_returns_Ret < Rhs, void, false > {
   BOOST_STATIC_CONSTANT(bool, value = false);
};

template < typename Rhs, typename Ret >
struct operator_returns_Ret < Rhs, Ret, true > {
   BOOST_STATIC_CONSTANT(bool, value = false);
};

// otherwise checks if it is convertible to Ret using the sizeof trick
// based on overload resolution
// condition: Ret!=void and Ret!=dont_care and the operator does not return void
template < typename Rhs, typename Ret >
struct operator_returns_Ret < Rhs, Ret, false > {
   static ::boost::type_traits::yes_type is_convertible_to_Ret(Ret); // this version is preferred for types convertible to Ret
   static ::boost::type_traits::no_type is_convertible_to_Ret(...); // this version is used otherwise

   BOOST_STATIC_CONSTANT(bool, value = (sizeof(is_convertible_to_Ret(BOOST_TT_TRAIT_OP make<Rhs>()))==sizeof(::boost::type_traits::yes_type)));
};


// 5. checks for operator existence
// condition: Rhs!=void

// checks if our definition of operator BOOST_TT_TRAIT_OP is used or an other
// existing one;
// this is done with redefinition of "operator," that returns no_operator or has_operator
struct has_operator { };
no_operator operator,(no_operator, has_operator);

template < typename Rhs >
struct operator_exists {
   static ::boost::type_traits::yes_type s_check(has_operator); // this version is preferred when operator exists
   static ::boost::type_traits::no_type s_check(no_operator); // this version is used otherwise

   BOOST_STATIC_CONSTANT(bool, value = (sizeof(s_check(((BOOST_TT_TRAIT_OP make<Rhs>()),make<has_operator>())))==sizeof(::boost::type_traits::yes_type)));
};


// 6. main trait: to avoid any compilation error, this class behaves
// differently when operator BOOST_TT_TRAIT_OP(Rhs) is forbidden by the
// standard.
// Forbidden_if is a bool that is:
// - true when the operator BOOST_TT_TRAIT_OP(Rhs) is forbidden by the standard
//   (would yield compilation error if used)
// - false otherwise
template < typename Rhs, typename Ret, bool Forbidden_if >
struct trait_impl1;

template < typename Rhs, typename Ret >
struct trait_impl1 < Rhs, Ret, true > {
   BOOST_STATIC_CONSTANT(bool, value = false);
};

template < typename Rhs, typename Ret >
struct trait_impl1 < Rhs, Ret, false > {
   BOOST_STATIC_CONSTANT(bool,
      value = (operator_exists < Rhs >::value && operator_returns_Ret < Rhs, Ret, operator_returns_void < Rhs >::value >::value));
};

// specialization needs to be declared for the special void case
template < typename Ret >
struct trait_impl1 < void, Ret, false > {
   BOOST_STATIC_CONSTANT(bool, value = false);
};

// defines some typedef for convenience
template < typename Rhs, typename Ret >
struct trait_impl {
   typedef typename ::boost::remove_reference<Rhs>::type Rhs_noref;
   typedef typename ::boost::remove_cv<Rhs_noref>::type Rhs_nocv;
   typedef typename ::boost::remove_cv< typename ::boost::remove_reference< typename ::boost::remove_pointer<Rhs_noref>::type >::type >::type Rhs_noptr;
   BOOST_STATIC_CONSTANT(bool, value = (trait_impl1 < Rhs_noref, Ret, BOOST_TT_FORBIDDEN_IF >::value));
};

} // namespace impl
} // namespace detail

// this is the accessible definition of the trait to end user
template <class Rhs, class Ret=::boost::detail::BOOST_JOIN(BOOST_TT_TRAIT_NAME,_impl)::dont_care>
struct BOOST_TT_TRAIT_NAME : public integral_constant<bool, (::boost::detail::BOOST_JOIN(BOOST_TT_TRAIT_NAME, _impl)::trait_impl < Rhs, Ret >::value)>{};

} // namespace boost

#if defined(BOOST_MSVC)
#   pragma warning ( pop )
#endif

#endif

