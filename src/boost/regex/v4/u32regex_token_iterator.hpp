/*
 *
 * Copyright (c) 2003
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         u32regex_token_iterator.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Provides u32regex_token_iterator implementation.
  */

#ifndef BOOST_REGEX_V4_U32REGEX_TOKEN_ITERATOR_HPP
#define BOOST_REGEX_V4_U32REGEX_TOKEN_ITERATOR_HPP

#if (BOOST_WORKAROUND(BOOST_BORLANDC, >= 0x560) && BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x570)))\
      || BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
//
// Borland C++ Builder 6, and Visual C++ 6,
// can't cope with the array template constructor
// so we have a template member that will accept any type as 
// argument, and then assert that is really is an array:
//
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_array.hpp>
#endif

namespace boost{

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable:4700)
#endif

template <class BidirectionalIterator>
class u32regex_token_iterator_implementation 
{
   typedef u32regex                              regex_type;
   typedef sub_match<BidirectionalIterator>      value_type;

   match_results<BidirectionalIterator> what;   // current match
   BidirectionalIterator                end;    // end of search area
   BidirectionalIterator                base;   // start of search area
   const regex_type                     re;     // the expression
   match_flag_type                      flags;  // match flags
   value_type                           result; // the current string result
   int                                  N;      // the current sub-expression being enumerated
   std::vector<int>                     subs;   // the sub-expressions to enumerate

public:
   u32regex_token_iterator_implementation(const regex_type* p, BidirectionalIterator last, int sub, match_flag_type f)
      : end(last), re(*p), flags(f){ subs.push_back(sub); }
   u32regex_token_iterator_implementation(const regex_type* p, BidirectionalIterator last, const std::vector<int>& v, match_flag_type f)
      : end(last), re(*p), flags(f), subs(v){}
#if (BOOST_WORKAROUND(__BORLANDC__, >= 0x560) && BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570)))\
      || BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003)) \
      || BOOST_WORKAROUND(__HP_aCC, < 60700)
   template <class T>
   u32regex_token_iterator_implementation(const regex_type* p, BidirectionalIterator last, const T& submatches, match_flag_type f)
      : end(last), re(*p), flags(f)
   {
      // assert that T really is an array:
      BOOST_STATIC_ASSERT(::boost::is_array<T>::value);
      const std::size_t array_size = sizeof(T) / sizeof(submatches[0]);
      for(std::size_t i = 0; i < array_size; ++i)
      {
         subs.push_back(submatches[i]);
      }
   }
#else
   template <std::size_t CN>
   u32regex_token_iterator_implementation(const regex_type* p, BidirectionalIterator last, const int (&submatches)[CN], match_flag_type f)
      : end(last), re(*p), flags(f)
   {
      for(std::size_t i = 0; i < CN; ++i)
      {
         subs.push_back(submatches[i]);
      }
   }
#endif

   bool init(BidirectionalIterator first)
   {
      base = first;
      N = 0;
      if(u32regex_search(first, end, what, re, flags, base) == true)
      {
         N = 0;
         result = ((subs[N] == -1) ? what.prefix() : what[(int)subs[N]]);
         return true;
      }
      else if((subs[N] == -1) && (first != end))
      {
         result.first = first;
         result.second = end;
         result.matched = (first != end);
         N = -1;
         return true;
      }
      return false;
   }
   bool compare(const u32regex_token_iterator_implementation& that)
   {
      if(this == &that) return true;
      return (&re.get_data() == &that.re.get_data()) 
         && (end == that.end) 
         && (flags == that.flags) 
         && (N == that.N) 
         && (what[0].first == that.what[0].first) 
         && (what[0].second == that.what[0].second);
   }
   const value_type& get()
   { return result; }
   bool next()
   {
      if(N == -1)
         return false;
      if(N+1 < (int)subs.size())
      {
         ++N;
         result =((subs[N] == -1) ? what.prefix() : what[subs[N]]);
         return true;
      }
      //if(what.prefix().first != what[0].second)
      //   flags |= match_prev_avail | regex_constants::match_not_bob;
      BidirectionalIterator last_end(what[0].second);
      if(u32regex_search(last_end, end, what, re, ((what[0].first == what[0].second) ? flags | regex_constants::match_not_initial_null : flags), base))
      {
         N =0;
         result =((subs[N] == -1) ? what.prefix() : what[subs[N]]);
         return true;
      }
      else if((last_end != end) && (subs[0] == -1))
      {
         N =-1;
         result.first = last_end;
         result.second = end;
         result.matched = (last_end != end);
         return true;
      }
      return false;
   }
private:
   u32regex_token_iterator_implementation& operator=(const u32regex_token_iterator_implementation&);
};

template <class BidirectionalIterator>
class u32regex_token_iterator 
{
private:
   typedef u32regex_token_iterator_implementation<BidirectionalIterator> impl;
   typedef shared_ptr<impl> pimpl;
public:
   typedef          u32regex                                                regex_type;
   typedef          sub_match<BidirectionalIterator>                        value_type;
   typedef typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<BidirectionalIterator>::difference_type 
                                                                            difference_type;
   typedef          const value_type*                                       pointer;
   typedef          const value_type&                                       reference; 
   typedef          std::forward_iterator_tag                               iterator_category;
   
   u32regex_token_iterator(){}
   u32regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re, 
                        int submatch = 0, match_flag_type m = match_default)
                        : pdata(new impl(&re, b, submatch, m))
   {
      if(!pdata->init(a))
         pdata.reset();
   }
   u32regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re, 
                        const std::vector<int>& submatches, match_flag_type m = match_default)
                        : pdata(new impl(&re, b, submatches, m))
   {
      if(!pdata->init(a))
         pdata.reset();
   }
#if (BOOST_WORKAROUND(__BORLANDC__, >= 0x560) && BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570)))\
      || BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003)) \
      || BOOST_WORKAROUND(__HP_aCC, < 60700)
   template <class T>
   u32regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re,
                        const T& submatches, match_flag_type m = match_default)
                        : pdata(new impl(&re, b, submatches, m))
   {
      if(!pdata->init(a))
         pdata.reset();
   }
#else
   template <std::size_t N>
   u32regex_token_iterator(BidirectionalIterator a, BidirectionalIterator b, const regex_type& re,
                        const int (&submatches)[N], match_flag_type m = match_default)
                        : pdata(new impl(&re, b, submatches, m))
   {
      if(!pdata->init(a))
         pdata.reset();
   }
#endif
   u32regex_token_iterator(const u32regex_token_iterator& that)
      : pdata(that.pdata) {}
   u32regex_token_iterator& operator=(const u32regex_token_iterator& that)
   {
      pdata = that.pdata;
      return *this;
   }
   bool operator==(const u32regex_token_iterator& that)const
   { 
      if((pdata.get() == 0) || (that.pdata.get() == 0))
         return pdata.get() == that.pdata.get();
      return pdata->compare(*(that.pdata.get())); 
   }
   bool operator!=(const u32regex_token_iterator& that)const
   { return !(*this == that); }
   const value_type& operator*()const
   { return pdata->get(); }
   const value_type* operator->()const
   { return &(pdata->get()); }
   u32regex_token_iterator& operator++()
   {
      cow();
      if(0 == pdata->next())
      {
         pdata.reset();
      }
      return *this;
   }
   u32regex_token_iterator operator++(int)
   {
      u32regex_token_iterator result(*this);
      ++(*this);
      return result;
   }
private:

   pimpl pdata;

   void cow()
   {
      // copy-on-write
      if(pdata.get() && !pdata.unique())
      {
         pdata.reset(new impl(*(pdata.get())));
      }
   }
};

typedef u32regex_token_iterator<const char*> utf8regex_token_iterator;
typedef u32regex_token_iterator<const UChar*> utf16regex_token_iterator;
typedef u32regex_token_iterator<const UChar32*> utf32regex_token_iterator;

// construction from an integral sub_match state_id:
inline u32regex_token_iterator<const char*> make_u32regex_token_iterator(const char* p, const u32regex& e, int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const char*>(p, p+std::strlen(p), e, submatch, m);
}
#ifndef BOOST_NO_WREGEX
inline u32regex_token_iterator<const wchar_t*> make_u32regex_token_iterator(const wchar_t* p, const u32regex& e, int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const wchar_t*>(p, p+std::wcslen(p), e, submatch, m);
}
#endif
#if !defined(BOOST_REGEX_UCHAR_IS_WCHAR_T)
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const UChar* p, const u32regex& e, int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(p, p+u_strlen(p), e, submatch, m);
}
#endif
template <class charT, class Traits, class Alloc>
inline u32regex_token_iterator<typename std::basic_string<charT, Traits, Alloc>::const_iterator> make_u32regex_token_iterator(const std::basic_string<charT, Traits, Alloc>& p, const u32regex& e, int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   typedef typename std::basic_string<charT, Traits, Alloc>::const_iterator iter_type;
   return u32regex_token_iterator<iter_type>(p.begin(), p.end(), e, submatch, m);
}
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const U_NAMESPACE_QUALIFIER UnicodeString& s, const u32regex& e, int submatch = 0, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(s.getBuffer(), s.getBuffer() + s.length(), e, submatch, m);
}

// construction from a reference to an array:
template <std::size_t N>
inline u32regex_token_iterator<const char*> make_u32regex_token_iterator(const char* p, const u32regex& e, const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const char*>(p, p+std::strlen(p), e, submatch, m);
}
#ifndef BOOST_NO_WREGEX
template <std::size_t N>
inline u32regex_token_iterator<const wchar_t*> make_u32regex_token_iterator(const wchar_t* p, const u32regex& e, const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const wchar_t*>(p, p+std::wcslen(p), e, submatch, m);
}
#endif
#if !defined(BOOST_REGEX_UCHAR_IS_WCHAR_T)
template <std::size_t N>
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const UChar* p, const u32regex& e, const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(p, p+u_strlen(p), e, submatch, m);
}
#endif
template <class charT, class Traits, class Alloc, std::size_t N>
inline u32regex_token_iterator<typename std::basic_string<charT, Traits, Alloc>::const_iterator> make_u32regex_token_iterator(const std::basic_string<charT, Traits, Alloc>& p, const u32regex& e, const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   typedef typename std::basic_string<charT, Traits, Alloc>::const_iterator iter_type;
   return u32regex_token_iterator<iter_type>(p.begin(), p.end(), e, submatch, m);
}
template <std::size_t N>
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const U_NAMESPACE_QUALIFIER UnicodeString& s, const u32regex& e, const int (&submatch)[N], regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(s.getBuffer(), s.getBuffer() + s.length(), e, submatch, m);
}

// construction from a vector of sub_match state_id's:
inline u32regex_token_iterator<const char*> make_u32regex_token_iterator(const char* p, const u32regex& e, const std::vector<int>& submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const char*>(p, p+std::strlen(p), e, submatch, m);
}
#ifndef BOOST_NO_WREGEX
inline u32regex_token_iterator<const wchar_t*> make_u32regex_token_iterator(const wchar_t* p, const u32regex& e, const std::vector<int>& submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const wchar_t*>(p, p+std::wcslen(p), e, submatch, m);
}
#endif
#if !defined(U_WCHAR_IS_UTF16) && (U_SIZEOF_WCHAR_T != 2)
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const UChar* p, const u32regex& e, const std::vector<int>& submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(p, p+u_strlen(p), e, submatch, m);
}
#endif
template <class charT, class Traits, class Alloc>
inline u32regex_token_iterator<typename std::basic_string<charT, Traits, Alloc>::const_iterator> make_u32regex_token_iterator(const std::basic_string<charT, Traits, Alloc>& p, const u32regex& e, const std::vector<int>& submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   typedef typename std::basic_string<charT, Traits, Alloc>::const_iterator iter_type;
   return u32regex_token_iterator<iter_type>(p.begin(), p.end(), e, submatch, m);
}
inline u32regex_token_iterator<const UChar*> make_u32regex_token_iterator(const U_NAMESPACE_QUALIFIER UnicodeString& s, const u32regex& e, const std::vector<int>& submatch, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_token_iterator<const UChar*>(s.getBuffer(), s.getBuffer() + s.length(), e, submatch, m);
}

#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

} // namespace boost

#endif // BOOST_REGEX_V4_REGEX_TOKEN_ITERATOR_HPP




