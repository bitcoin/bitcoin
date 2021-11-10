//  (C) Copyright John Maddock 2015. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPE_TRAITS_INTEGRAL_CONSTANT_HPP
#define BOOST_TYPE_TRAITS_INTEGRAL_CONSTANT_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if (BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1400)) \
   || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x610)) \
   || BOOST_WORKAROUND(__DMC__, BOOST_TESTED_AT(0x840)) \
   || BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3202)) \
   || BOOST_WORKAROUND(BOOST_INTEL_CXX_VERSION, BOOST_TESTED_AT(810)) )\
   || defined(BOOST_MPL_CFG_NO_ADL_BARRIER_NAMESPACE)


namespace boost{
   namespace mpl
   {
      template <bool B> struct bool_;
      template <class I, I val> struct integral_c;
      struct integral_c_tag;
   }
}

#else

namespace mpl_{

   template <bool B> struct bool_;
   template <class I, I val> struct integral_c;
   struct integral_c_tag;
}

namespace boost
{
   namespace mpl
   {
      using ::mpl_::bool_;
      using ::mpl_::integral_c;
      using ::mpl_::integral_c_tag;
   }
}

#endif

namespace boost{

   template <class T, T val>
   struct integral_constant
   {
      typedef mpl::integral_c_tag tag;
      typedef T value_type;
      typedef integral_constant<T, val> type;
      static const T value = val;

      operator const mpl::integral_c<T, val>& ()const
      {
         static const char data[sizeof(long)] = { 0 };
         static const void* pdata = data;
         return *(reinterpret_cast<const mpl::integral_c<T, val>*>(pdata));
      }
      BOOST_CONSTEXPR operator T()const { return val; }
   };

   template <class T, T val>
   T const integral_constant<T, val>::value;
      
   template <bool val>
   struct integral_constant<bool, val>
   {
      typedef mpl::integral_c_tag tag;
      typedef bool value_type;
      typedef integral_constant<bool, val> type;
      static const bool value = val;

      operator const mpl::bool_<val>& ()const
      {
         static const char data[sizeof(long)] = { 0 };
         static const void* pdata = data;
         return *(reinterpret_cast<const mpl::bool_<val>*>(pdata));
      }
      BOOST_CONSTEXPR operator bool()const { return val; }
   };

   template <bool val>
   bool const integral_constant<bool, val>::value;

   typedef integral_constant<bool, true> true_type;
   typedef integral_constant<bool, false> false_type;

}

#endif
