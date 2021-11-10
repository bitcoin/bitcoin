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
  *   FILE         u32regex_iterator.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Provides u32regex_iterator implementation.
  */

#ifndef BOOST_REGEX_V4_U32REGEX_ITERATOR_HPP
#define BOOST_REGEX_V4_U32REGEX_ITERATOR_HPP

namespace boost{

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

template <class BidirectionalIterator>
class u32regex_iterator_implementation 
{
   typedef u32regex regex_type;

   match_results<BidirectionalIterator> what;  // current match
   BidirectionalIterator                base;  // start of sequence
   BidirectionalIterator                end;   // end of sequence
   const regex_type                     re;   // the expression
   match_flag_type                      flags; // flags for matching

public:
   u32regex_iterator_implementation(const regex_type* p, BidirectionalIterator last, match_flag_type f)
      : base(), end(last), re(*p), flags(f){}
   bool init(BidirectionalIterator first)
   {
      base = first;
      return u32regex_search(first, end, what, re, flags, base);
   }
   bool compare(const u32regex_iterator_implementation& that)
   {
      if(this == &that) return true;
      return (&re.get_data() == &that.re.get_data()) && (end == that.end) && (flags == that.flags) && (what[0].first == that.what[0].first) && (what[0].second == that.what[0].second);
   }
   const match_results<BidirectionalIterator>& get()
   { return what; }
   bool next()
   {
      //if(what.prefix().first != what[0].second)
      //   flags |= match_prev_avail;
      BidirectionalIterator next_start = what[0].second;
      match_flag_type f(flags);
      if(!what.length())
         f |= regex_constants::match_not_initial_null;
      //if(base != next_start)
      //   f |= regex_constants::match_not_bob;
      bool result = u32regex_search(next_start, end, what, re, f, base);
      if(result)
         what.set_base(base);
      return result;
   }
private:
   u32regex_iterator_implementation& operator=(const u32regex_iterator_implementation&);
};

template <class BidirectionalIterator>
class u32regex_iterator 
{
private:
   typedef u32regex_iterator_implementation<BidirectionalIterator> impl;
   typedef shared_ptr<impl> pimpl;
public:
   typedef          u32regex                                                regex_type;
   typedef          match_results<BidirectionalIterator>                    value_type;
   typedef typename BOOST_REGEX_DETAIL_NS::regex_iterator_traits<BidirectionalIterator>::difference_type 
                                                                            difference_type;
   typedef          const value_type*                                       pointer;
   typedef          const value_type&                                       reference; 
   typedef          std::forward_iterator_tag                               iterator_category;
   
   u32regex_iterator(){}
   u32regex_iterator(BidirectionalIterator a, BidirectionalIterator b, 
                  const regex_type& re, 
                  match_flag_type m = match_default)
                  : pdata(new impl(&re, b, m))
   {
      if(!pdata->init(a))
      {
         pdata.reset();
      }
   }
   u32regex_iterator(const u32regex_iterator& that)
      : pdata(that.pdata) {}
   u32regex_iterator& operator=(const u32regex_iterator& that)
   {
      pdata = that.pdata;
      return *this;
   }
   bool operator==(const u32regex_iterator& that)const
   { 
      if((pdata.get() == 0) || (that.pdata.get() == 0))
         return pdata.get() == that.pdata.get();
      return pdata->compare(*(that.pdata.get())); 
   }
   bool operator!=(const u32regex_iterator& that)const
   { return !(*this == that); }
   const value_type& operator*()const
   { return pdata->get(); }
   const value_type* operator->()const
   { return &(pdata->get()); }
   u32regex_iterator& operator++()
   {
      cow();
      if(0 == pdata->next())
      {
         pdata.reset();
      }
      return *this;
   }
   u32regex_iterator operator++(int)
   {
      u32regex_iterator result(*this);
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

typedef u32regex_iterator<const char*> utf8regex_iterator;
typedef u32regex_iterator<const UChar*> utf16regex_iterator;
typedef u32regex_iterator<const UChar32*> utf32regex_iterator;

inline u32regex_iterator<const char*> make_u32regex_iterator(const char* p, const u32regex& e, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_iterator<const char*>(p, p+std::strlen(p), e, m);
}
#ifndef BOOST_NO_WREGEX
inline u32regex_iterator<const wchar_t*> make_u32regex_iterator(const wchar_t* p, const u32regex& e, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_iterator<const wchar_t*>(p, p+std::wcslen(p), e, m);
}
#endif
#if !defined(BOOST_REGEX_UCHAR_IS_WCHAR_T)
inline u32regex_iterator<const UChar*> make_u32regex_iterator(const UChar* p, const u32regex& e, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_iterator<const UChar*>(p, p+u_strlen(p), e, m);
}
#endif
template <class charT, class Traits, class Alloc>
inline u32regex_iterator<typename std::basic_string<charT, Traits, Alloc>::const_iterator> make_u32regex_iterator(const std::basic_string<charT, Traits, Alloc>& p, const u32regex& e, regex_constants::match_flag_type m = regex_constants::match_default)
{
   typedef typename std::basic_string<charT, Traits, Alloc>::const_iterator iter_type;
   return u32regex_iterator<iter_type>(p.begin(), p.end(), e, m);
}
inline u32regex_iterator<const UChar*> make_u32regex_iterator(const U_NAMESPACE_QUALIFIER UnicodeString& s, const u32regex& e, regex_constants::match_flag_type m = regex_constants::match_default)
{
   return u32regex_iterator<const UChar*>(s.getBuffer(), s.getBuffer() + s.length(), e, m);
}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

} // namespace boost

#endif // BOOST_REGEX_V4_REGEX_ITERATOR_HPP

