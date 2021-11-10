//  (C) Copyright 2009-2011 Frederic Bron.
//
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_HAS_DEREFERENCE_HPP_INCLUDED
#define BOOST_TT_HAS_DEREFERENCE_HPP_INCLUDED

#define BOOST_TT_TRAIT_NAME has_dereference
#define BOOST_TT_TRAIT_OP *
#define BOOST_TT_FORBIDDEN_IF\
   /* void* or fundamental */\
   (\
      (\
         ::boost::is_pointer< Rhs_noref >::value && \
         ::boost::is_void< Rhs_noptr >::value\
      ) || \
      ::boost::is_fundamental< Rhs_nocv >::value\
   )


#include <boost/type_traits/detail/has_prefix_operator.hpp>

#undef BOOST_TT_TRAIT_NAME
#undef BOOST_TT_TRAIT_OP
#undef BOOST_TT_FORBIDDEN_IF
#if defined(BOOST_TT_HAS_ACCURATE_BINARY_OPERATOR_DETECTION)

namespace boost {

   template <class R>
   struct has_dereference<void*, R> : public false_type {};
   template <>
   struct has_dereference<void*, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*, R> : public false_type {};
   template <>
   struct has_dereference<const void*, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const, R> : public false_type {};
   template <>
   struct has_dereference<void*const, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const, R> : public false_type {};
   template <>
   struct has_dereference<const void*const, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const, void> : public false_type {};

   template <class R>
   struct has_dereference<void*volatile, R> : public false_type {};
   template <>
   struct has_dereference<void*volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*volatile, R> : public false_type {};
   template <>
   struct has_dereference<const void*volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*volatile, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*volatile, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const volatile, R> : public false_type {};
   template <>
   struct has_dereference<void*const volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const volatile, R> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const volatile, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const volatile, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile, void> : public false_type {};

   // references:
   template <class R>
   struct has_dereference<void*&, R> : public false_type {};
   template <>
   struct has_dereference<void*&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*&, R> : public false_type {};
   template <>
   struct has_dereference<const void*&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const&, R> : public false_type {};
   template <>
   struct has_dereference<void*const&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const&, R> : public false_type {};
   template <>
   struct has_dereference<const void*const&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*volatile&, R> : public false_type {};
   template <>
   struct has_dereference<void*volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*volatile&, R> : public false_type {};
   template <>
   struct has_dereference<const void*volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*volatile&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*volatile&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const volatile&, R> : public false_type {};
   template <>
   struct has_dereference<void*const volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const volatile&, R> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const volatile&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const volatile&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile&, void> : public false_type {};

   // rvalue refs:
   template <class R>
   struct has_dereference<void*&&, R> : public false_type {};
   template <>
   struct has_dereference<void*&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*&&, R> : public false_type {};
   template <>
   struct has_dereference<const void*&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*&&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*&&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*&&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*&&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const&&, R> : public false_type {};
   template <>
   struct has_dereference<void*const&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const&&, R> : public false_type {};
   template <>
   struct has_dereference<const void*const&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const&&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const&&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const&&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const&&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<void*volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<const void*volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<void*const volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<void*const volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<void*const volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const void*const volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const void*const volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<volatile void*const volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<volatile void*const volatile&&, void> : public false_type {};

   template <class R>
   struct has_dereference<const volatile void*const volatile&&, R> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile&&, boost::binary_op_detail::dont_care> : public false_type {};
   template <>
   struct has_dereference<const volatile void*const volatile&&, void> : public false_type {};


}
#endif
#endif
