/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE:        primary_transform.hpp
  *   VERSION:     see <boost/version.hpp>
  *   DESCRIPTION: Heuristically determines the sort string format in use
  *                by the current locale.
  */

#ifndef BOOST_REGEX_PRIMARY_TRANSFORM
#define BOOST_REGEX_PRIMARY_TRANSFORM

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

namespace boost{
   namespace BOOST_REGEX_DETAIL_NS{


enum{
   sort_C,
   sort_fixed,
   sort_delim,
   sort_unknown
};

template <class S, class charT>
unsigned count_chars(const S& s, charT c)
{
   //
   // Count how many occurrences of character c occur
   // in string s: if c is a delimeter between collation
   // fields, then this should be the same value for all
   // sort keys:
   //
   unsigned int count = 0;
   for(unsigned pos = 0; pos < s.size(); ++pos)
   {
      if(s[pos] == c) ++count;
   }
   return count;
}


template <class traits, class charT>
unsigned find_sort_syntax(const traits* pt, charT* delim)
{
   //
   // compare 'a' with 'A' to see how similar they are,
   // should really use a-accute but we can't portably do that,
   //
   typedef typename traits::string_type string_type;
   typedef typename traits::char_type char_type;

   // Suppress incorrect warning for MSVC
   (void)pt;

   char_type a[2] = {'a', '\0', };
   string_type sa(pt->transform(a, a+1));
   if(sa == a)
   {
      *delim = 0;
      return sort_C;
   }
   char_type A[2] = { 'A', '\0', };
   string_type sA(pt->transform(A, A+1));
   char_type c[2] = { ';', '\0', };
   string_type sc(pt->transform(c, c+1));

   int pos = 0;
   while((pos <= static_cast<int>(sa.size())) && (pos <= static_cast<int>(sA.size())) && (sa[pos] == sA[pos])) ++pos;
   --pos;
   if(pos < 0)
   {
      *delim = 0;
      return sort_unknown;
   }
   //
   // at this point sa[pos] is either the end of a fixed width field
   // or the character that acts as a delimiter:
   //
   charT maybe_delim = sa[pos];
   if((pos != 0) && (count_chars(sa, maybe_delim) == count_chars(sA, maybe_delim)) && (count_chars(sa, maybe_delim) == count_chars(sc, maybe_delim)))
   {
      *delim = maybe_delim;
      return sort_delim;
   }
   //
   // OK doen't look like a delimiter, try for fixed width field:
   //
   if((sa.size() == sA.size()) && (sa.size() == sc.size()))
   {
      // note assumes that the fixed width field is less than
      // (numeric_limits<charT>::max)(), should be true for all types
      // I can't imagine 127 character fields...
      *delim = static_cast<charT>(++pos);
      return sort_fixed;
   }
   //
   // don't know what it is:
   //
   *delim = 0;
   return sort_unknown;
}


   } // namespace BOOST_REGEX_DETAIL_NS
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif







