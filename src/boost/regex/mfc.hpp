/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         mfc.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Overloads and helpers for using MFC/ATL string types with Boost.Regex.
  */

#ifndef BOOST_REGEX_MFC_HPP
#define BOOST_REGEX_MFC_HPP

#include <atlsimpstr.h>
#include <boost/regex.hpp>

namespace boost{

//
// define the types used for TCHAR's:
typedef basic_regex<TCHAR> tregex;
typedef match_results<TCHAR const*> tmatch;
typedef regex_iterator<TCHAR const*> tregex_iterator;
typedef regex_token_iterator<TCHAR const*> tregex_token_iterator;

// Obsolete. Remove
#define SIMPLE_STRING_PARAM class B, bool b
#define SIMPLE_STRING_ARG_LIST B, b

//
// define regex creation functions:
//
template <class B, bool b>
inline basic_regex<B> 
make_regex(const ATL::CSimpleStringT<B, b>& s, ::boost::regex_constants::syntax_option_type f = boost::regex_constants::normal)
{
   basic_regex<B> result(s.GetString(), s.GetString() + s.GetLength(), f);
   return result;
}
//
// regex_match overloads:
//
template <class B, bool b, class A, class T>
inline bool regex_match(const ATL::CSimpleStringT<B, b>& s,
                 match_results<const B*, A>& what,
                 const basic_regex<B, T>& e,
                 boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   return ::boost::regex_match(s.GetString(),
                               s.GetString() + s.GetLength(),
                               what,
                               e,
                               f);
}

template <class B, bool b, class T>
inline bool regex_match(const ATL::CSimpleStringT<B, b>& s,
                 const basic_regex<B, T>& e,
                 boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   return ::boost::regex_match(s.GetString(),
                               s.GetString() + s.GetLength(),
                               e,
                               f);
}
//
// regex_search overloads:
//
template <class B, bool b, class A, class T>
inline bool regex_search(const ATL::CSimpleStringT<B, b>& s,
                 match_results<const B*, A>& what,
                 const basic_regex<B, T>& e,
                 boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   return ::boost::regex_search(s.GetString(),
                               s.GetString() + s.GetLength(),
                               what,
                               e,
                               f);
}

template <class B, bool b, class T>
inline bool regex_search(const ATL::CSimpleStringT<B, b>& s,
                 const basic_regex<B, T>& e,
                 boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   return ::boost::regex_search(s.GetString(),
                               s.GetString() + s.GetLength(),
                               e,
                               f);
}
//
// regex_iterator creation:
//
template <class B, bool b>
inline regex_iterator<B const*> 
make_regex_iterator(const ATL::CSimpleStringT<B, b>& s, const basic_regex<B>& e, ::boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   regex_iterator<B const*> result(s.GetString(), s.GetString() + s.GetLength(), e, f);
   return result;
}

template <class B, bool b>
inline regex_token_iterator<B const*> 
   make_regex_token_iterator(const ATL::CSimpleStringT<B, b>& s, const basic_regex<B>& e, int sub = 0, ::boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   regex_token_iterator<B const*> result(s.GetString(), s.GetString() + s.GetLength(), e, sub, f);
   return result;
}

template <class B, bool b>
inline regex_token_iterator<B const*> 
make_regex_token_iterator(const ATL::CSimpleStringT<B, b>& s, const basic_regex<B>& e, const std::vector<int>& subs, ::boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   regex_token_iterator<B const*> result(s.GetString(), s.GetString() + s.GetLength(), e, subs, f);
   return result;
}

template <class B, bool b, std::size_t N>
inline regex_token_iterator<B const*> 
make_regex_token_iterator(const ATL::CSimpleStringT<B, b>& s, const basic_regex<B>& e, const int (& subs)[N], ::boost::regex_constants::match_flag_type f = boost::regex_constants::match_default)
{
   regex_token_iterator<B const*> result(s.GetString(), s.GetString() + s.GetLength(), e, subs, f);
   return result;
}

template <class OutputIterator, class BidirectionalIterator, class traits,
          class B, bool b>
OutputIterator regex_replace(OutputIterator out,
                           BidirectionalIterator first,
                           BidirectionalIterator last,
                           const basic_regex<B, traits>& e,
                           const ATL::CSimpleStringT<B, b>& fmt,
                           match_flag_type flags = match_default)
{
   return ::boost::regex_replace(out, first, last, e, fmt.GetString(), flags);
}

namespace BOOST_REGEX_DETAIL_NS{

template <class B, bool b>
class mfc_string_out_iterator
{
   ATL::CSimpleStringT<B, b>* out;
public:
   mfc_string_out_iterator(ATL::CSimpleStringT<B, b>& s) : out(&s) {}
   mfc_string_out_iterator& operator++() { return *this; }
   mfc_string_out_iterator& operator++(int) { return *this; }
   mfc_string_out_iterator& operator*() { return *this; }
   mfc_string_out_iterator& operator=(B v) 
   { 
      out->AppendChar(v); 
      return *this; 
   }
   typedef std::ptrdiff_t difference_type;
   typedef B value_type;
   typedef value_type* pointer;
   typedef value_type& reference;
   typedef std::output_iterator_tag iterator_category;
};

}

template <class traits, class B, bool b>
ATL::CSimpleStringT<B, b> regex_replace(const ATL::CSimpleStringT<B, b>& s,
                            const basic_regex<B, traits>& e,
                            const ATL::CSimpleStringT<B, b>& fmt,
                            match_flag_type flags = match_default)
{
   ATL::CSimpleStringT<B, b> result(s.GetManager());
   BOOST_REGEX_DETAIL_NS::mfc_string_out_iterator<B, b> i(result);
   regex_replace(i, s.GetString(), s.GetString() + s.GetLength(), e, fmt.GetString(), flags);
   return result;
}

} // namespace boost.

#endif
