//  (C) Copyright Daniel Frey and Robert Ramey 2009.
//  (C) Copyright Balint Cserni 2017
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.
 
#ifndef BOOST_TT_IS_VIRTUAL_BASE_OF_HPP_INCLUDED
#define BOOST_TT_IS_VIRTUAL_BASE_OF_HPP_INCLUDED

#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/make_void.hpp>
#include <utility>

namespace boost {
   namespace detail {


#ifdef BOOST_MSVC
#pragma warning( push )
#pragma warning( disable : 4584 4250 4594)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#pragma GCC system_header
#endif

#if !defined(BOOST_NO_SFINAE_EXPR) && !defined(BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS) && !defined(BOOST_NO_CXX11_NULLPTR) && !BOOST_WORKAROUND(BOOST_GCC, < 40800)

      // Implementation based on the standard's rules of explicit type conversions.
      // A pointer to an object of *derived* class type may be explicitly converted to a pointer to an *unambiguous* *base* class type.
      // A pointer to an object of an *unambiguous* *non-virtual* *base* class type may be explicitly converted to a pointer of a *derived* class type.
      // Therefore Derived has a virtual base Base if and only if
      //   (1) a Derived* can be converted to Base* (so the base class is unambiguous, which comes necessarily from virtual inheritance)
      //   (2) a Base* cannot be converted to Derived* (so the base class is either ambiguous or virtual)
      // With both conditions true, Base must be a virtual base of Derived.
      // The "is_base_of" is only needed so the compiler can (but is not required to) error out if the types are incomplete.
      // This is in league with the the expected behaviour.

      template<class T, class U>
      constexpr bool is_virtual_base_impl(...) { return true; }

      // C-style casts have the power to ignore inheritance visibility while still act as a static_cast.
      // They can also fall back to the behaviour of reinterpret_cast, which allows is_virtual_base_of to work on non-class types too.
      // Note that because we are casting pointers there can be no user-defined operators to interfere.
      template<class T, class U,
         typename boost::make_void<decltype((U*)(std::declval<T*>()))>::type* =
         nullptr>
         constexpr bool is_virtual_base_impl(int) { return false; }

   } // namespace detail

   template<class T, class U>
   struct is_virtual_base_of : public
      boost::integral_constant<
      bool,
      boost::is_base_of<T, U>::value &&
      detail::is_virtual_base_impl<T, U>(0) &&
      !detail::is_virtual_base_impl<U, T>(0)
      > {};

#else

   template<typename Base, typename Derived, typename tag>
   struct is_virtual_base_of_impl
   {
      BOOST_STATIC_CONSTANT(bool, value = false);
   };

   template<typename Base, typename Derived>
   struct is_virtual_base_of_impl<Base, Derived, true_type>
   {
      union max_align
      {
         unsigned u;
         unsigned long ul;
         void* v;
         double d;
         long double ld;
#ifndef BOOST_NO_LONG_LONG
         long long ll;
#endif
      };
#ifdef BOOST_BORLANDC
      struct boost_type_traits_internal_struct_X : public virtual Derived, public virtual Base
      {
         boost_type_traits_internal_struct_X();
         boost_type_traits_internal_struct_X(const boost_type_traits_internal_struct_X&);
         boost_type_traits_internal_struct_X& operator=(const boost_type_traits_internal_struct_X&);
         ~boost_type_traits_internal_struct_X()throw();
         max_align data[4];
      };
      struct boost_type_traits_internal_struct_Y : public virtual Derived
      {
         boost_type_traits_internal_struct_Y();
         boost_type_traits_internal_struct_Y(const boost_type_traits_internal_struct_Y&);
         boost_type_traits_internal_struct_Y& operator=(const boost_type_traits_internal_struct_Y&);
         ~boost_type_traits_internal_struct_Y()throw();
         max_align data[4];
      };
#else
      struct boost_type_traits_internal_struct_X : public Derived, virtual Base
      {
         boost_type_traits_internal_struct_X();
         boost_type_traits_internal_struct_X(const boost_type_traits_internal_struct_X&);
         boost_type_traits_internal_struct_X& operator=(const boost_type_traits_internal_struct_X&);
         ~boost_type_traits_internal_struct_X()throw();
         max_align data[16];
      };
      struct boost_type_traits_internal_struct_Y : public Derived
      {
         boost_type_traits_internal_struct_Y();
         boost_type_traits_internal_struct_Y(const boost_type_traits_internal_struct_Y&);
         boost_type_traits_internal_struct_Y& operator=(const boost_type_traits_internal_struct_Y&);
         ~boost_type_traits_internal_struct_Y()throw();
         max_align data[16];
      };
#endif
      BOOST_STATIC_CONSTANT(bool, value = (sizeof(boost_type_traits_internal_struct_X) == sizeof(boost_type_traits_internal_struct_Y)));
   };

   template<typename Base, typename Derived>
   struct is_virtual_base_of_impl2
   {
      typedef boost::integral_constant<bool, (boost::is_base_of<Base, Derived>::value && !boost::is_same<Base, Derived>::value)> tag_type;
      typedef is_virtual_base_of_impl<Base, Derived, tag_type> imp;
      BOOST_STATIC_CONSTANT(bool, value = imp::value);
   };

} // namespace detail

template <class Base, class Derived> struct is_virtual_base_of : public integral_constant<bool, (::boost::detail::is_virtual_base_of_impl2<Base, Derived>::value)> {};

#endif

template <class Base, class Derived> struct is_virtual_base_of<Base&, Derived> : public false_type{};
template <class Base, class Derived> struct is_virtual_base_of<Base, Derived&> : public false_type{};
template <class Base, class Derived> struct is_virtual_base_of<Base&, Derived&> : public false_type{};

#ifdef BOOST_MSVC
#pragma warning( pop )
#endif

} // namespace boost

#endif
