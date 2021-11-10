///////////////////////////////////////////////////////////////
//  Copyright 2019 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MP_CONSTEXPR_HPP
#define BOOST_MP_CONSTEXPR_HPP

#include <boost/config.hpp>

namespace boost {

namespace multiprecision {

namespace std_constexpr {

template <class T>
inline BOOST_CXX14_CONSTEXPR void swap(T& a, T& b)
{
   T t(a);
   a = b;
   b = t;
}

template <class InputIterator, class OutputIterator>
inline BOOST_CXX14_CONSTEXPR OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
{
   //
   // There are 3 branches here, only one of which is selected at compile time:
   //
#ifndef BOOST_MP_NO_CONSTEXPR_DETECTION
   if (BOOST_MP_IS_CONST_EVALUATED(*first))
   {
      // constexpr safe code, never generates runtime code:
      while (first != last)
      {
         *result = *first;
         ++first;
         ++result;
      }
      return result;
   }
   else
#endif
   {
#ifndef BOOST_NO_CXX17_IF_CONSTEXPR
      if constexpr (std::is_pointer<InputIterator>::value && std::is_pointer<OutputIterator>::value && std::is_trivially_copyable<typename std::remove_reference<decltype(*first)>::type>::value)
      {
         // The normal runtime branch:
         std::memcpy(result, first, (last - first) * sizeof(*first));
         return result + (last - first);
      }
      else
#endif
      {
         // Alternate runtime branch:
         while (first != last)
         {
            *result = *first;
            ++first;
            ++result;
         }
         return result;
      }
   }
}

template <class I>
inline BOOST_CXX14_CONSTEXPR bool equal(const I* first, const I* last, const I* other)
{
   while (first != last)
   {
      if (*first != *other)
         return false;
      ++first;
      ++other;
   }
   return true;
}

}

}

} // namespace boost::multiprecision::std_constexpr

#endif
