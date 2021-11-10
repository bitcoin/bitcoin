/*
 *
 * Copyright (c) 1998-2009
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         match_results.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares template class match_results.
  */

#ifndef BOOST_REGEX_V5_MATCH_RESULTS_HPP
#define BOOST_REGEX_V5_MATCH_RESULTS_HPP

namespace boost{
#ifdef BOOST_REGEX_MSVC
#pragma warning(push)
#pragma warning(disable : 4251 4459)
#if BOOST_REGEX_MSVC < 1700
#     pragma warning(disable : 4231)
#endif
#  if BOOST_REGEX_MSVC < 1600
#     pragma warning(disable : 4660)
#  endif
#endif

namespace BOOST_REGEX_DETAIL_NS{

class named_subexpressions;

}

template <class BidiIterator, class Allocator>
class match_results
{ 
private:
   typedef          std::vector<sub_match<BidiIterator>, Allocator> vector_type;
public: 
   typedef          sub_match<BidiIterator>                         value_type;
   typedef typename std::allocator_traits<Allocator>::value_type const &    const_reference;
   typedef          const_reference                                         reference;
   typedef typename vector_type::const_iterator                             const_iterator;
   typedef          const_iterator                                          iterator;
   typedef typename std::iterator_traits<
                                    BidiIterator>::difference_type          difference_type;
   typedef typename std::allocator_traits<Allocator>::size_type             size_type;
   typedef          Allocator                                               allocator_type;
   typedef typename std::iterator_traits<
                                    BidiIterator>::value_type               char_type;
   typedef          std::basic_string<char_type>                            string_type;
   typedef          BOOST_REGEX_DETAIL_NS::named_subexpressions             named_sub_type;

   // construct/copy/destroy:
   explicit match_results(const Allocator& a = Allocator())
      : m_subs(a), m_base(), m_null(), m_last_closed_paren(0), m_is_singular(true) {}
   //
   // IMPORTANT: in the code below, the crazy looking checks around m_is_singular are
   // all required because it is illegal to copy a singular iterator.
   // See https://svn.boost.org/trac/boost/ticket/3632.
   //
   match_results(const match_results& m)
      : m_subs(m.m_subs), m_base(), m_null(), m_named_subs(m.m_named_subs), m_last_closed_paren(m.m_last_closed_paren), m_is_singular(m.m_is_singular)
   {
      if(!m_is_singular)
      {
         m_base = m.m_base;
         m_null = m.m_null;
      }
   }
   match_results& operator=(const match_results& m)
   {
      m_subs = m.m_subs;
      m_named_subs = m.m_named_subs;
      m_last_closed_paren = m.m_last_closed_paren;
      m_is_singular = m.m_is_singular;
      if(!m_is_singular)
      {
         m_base = m.m_base;
         m_null = m.m_null;
      }
      return *this;
   }
   ~match_results(){}

   // size:
   size_type size() const
   { return empty() ? 0 : m_subs.size() - 2; }
   size_type max_size() const
   { return m_subs.max_size(); }
   bool empty() const
   { return m_subs.size() < 2; }
   // element access:
   difference_type length(int sub = 0) const
   {
      if(m_is_singular)
         raise_logic_error();
      sub += 2;
      if((sub < (int)m_subs.size()) && (sub > 0))
         return m_subs[sub].length();
      return 0;
   }
   difference_type length(const char_type* sub) const
   {
      if(m_is_singular)
         raise_logic_error();
      const char_type* sub_end = sub;
      while(*sub_end) ++sub_end;
      return length(named_subexpression_index(sub, sub_end));
   }
   template <class charT>
   difference_type length(const charT* sub) const
   {
      if(m_is_singular)
         raise_logic_error();
      const charT* sub_end = sub;
      while(*sub_end) ++sub_end;
      return length(named_subexpression_index(sub, sub_end));
   }
   template <class charT, class Traits, class A>
   difference_type length(const std::basic_string<charT, Traits, A>& sub) const
   {
      return length(sub.c_str());
   }
   difference_type position(size_type sub = 0) const
   {
      if(m_is_singular)
         raise_logic_error();
      sub += 2;
      if(sub < m_subs.size())
      {
         const sub_match<BidiIterator>& s = m_subs[sub];
         if(s.matched || (sub == 2))
         {
            return std::distance((BidiIterator)(m_base), (BidiIterator)(s.first));
         }
      }
      return ~static_cast<difference_type>(0);
   }
   difference_type position(const char_type* sub) const
   {
      const char_type* sub_end = sub;
      while(*sub_end) ++sub_end;
      return position(named_subexpression_index(sub, sub_end));
   }
   template <class charT>
   difference_type position(const charT* sub) const
   {
      const charT* sub_end = sub;
      while(*sub_end) ++sub_end;
      return position(named_subexpression_index(sub, sub_end));
   }
   template <class charT, class Traits, class A>
   difference_type position(const std::basic_string<charT, Traits, A>& sub) const
   {
      return position(sub.c_str());
   }
   string_type str(int sub = 0) const
   {
      if(m_is_singular)
         raise_logic_error();
      sub += 2;
      string_type result;
      if(sub < (int)m_subs.size() && (sub > 0))
      {
         const sub_match<BidiIterator>& s = m_subs[sub];
         if(s.matched)
         {
            result = s.str();
         }
      }
      return result;
   }
   string_type str(const char_type* sub) const
   {
      return (*this)[sub].str();
   }
   template <class Traits, class A>
   string_type str(const std::basic_string<char_type, Traits, A>& sub) const
   {
      return (*this)[sub].str();
   }
   template <class charT>
   string_type str(const charT* sub) const
   {
      return (*this)[sub].str();
   }
   template <class charT, class Traits, class A>
   string_type str(const std::basic_string<charT, Traits, A>& sub) const
   {
      return (*this)[sub].str();
   }
   const_reference operator[](int sub) const
   {
      if(m_is_singular && m_subs.empty())
         raise_logic_error();
      sub += 2;
      if(sub < (int)m_subs.size() && (sub >= 0))
      {
         return m_subs[sub];
      }
      return m_null;
   }
   //
   // Named sub-expressions:
   //
   const_reference named_subexpression(const char_type* i, const char_type* j) const
   {
      //
      // Scan for the leftmost *matched* subexpression with the specified named:
      //
      if(m_is_singular)
         raise_logic_error();
      BOOST_REGEX_DETAIL_NS::named_subexpressions::range_type r = m_named_subs->equal_range(i, j);
      while((r.first != r.second) && ((*this)[r.first->index].matched == false))
         ++r.first;
      return r.first != r.second ? (*this)[r.first->index] : m_null;
   }
   template <class charT>
   const_reference named_subexpression(const charT* i, const charT* j) const
   {
      static_assert(sizeof(charT) <= sizeof(char_type), "Failed internal logic");
      if(i == j)
         return m_null;
      std::vector<char_type> s;
      while(i != j)
         s.insert(s.end(), *i++);
      return named_subexpression(&*s.begin(), &*s.begin() + s.size());
   }
   int named_subexpression_index(const char_type* i, const char_type* j) const
   {
      //
      // Scan for the leftmost *matched* subexpression with the specified named.
      // If none found then return the leftmost expression with that name,
      // otherwise an invalid index:
      //
      if(m_is_singular)
         raise_logic_error();
      BOOST_REGEX_DETAIL_NS::named_subexpressions::range_type s, r;
      s = r = m_named_subs->equal_range(i, j);
      while((r.first != r.second) && ((*this)[r.first->index].matched == false))
         ++r.first;
      if(r.first == r.second)
         r = s;
      return r.first != r.second ? r.first->index : -20;
   }
   template <class charT>
   int named_subexpression_index(const charT* i, const charT* j) const
   {
      static_assert(sizeof(charT) <= sizeof(char_type), "Failed internal logic");
      if(i == j)
         return -20;
      std::vector<char_type> s;
      while(i != j)
         s.insert(s.end(), *i++);
      return named_subexpression_index(&*s.begin(), &*s.begin() + s.size());
   }
   template <class Traits, class A>
   const_reference operator[](const std::basic_string<char_type, Traits, A>& s) const
   {
      return named_subexpression(s.c_str(), s.c_str() + s.size());
   }
   const_reference operator[](const char_type* p) const
   {
      const char_type* e = p;
      while(*e) ++e;
      return named_subexpression(p, e);
   }

   template <class charT>
   const_reference operator[](const charT* p) const
   {
      static_assert(sizeof(charT) <= sizeof(char_type), "Failed internal logic");
      if(*p == 0)
         return m_null;
      std::vector<char_type> s;
      while(*p)
         s.insert(s.end(), *p++);
      return named_subexpression(&*s.begin(), &*s.begin() + s.size());
   }
   template <class charT, class Traits, class A>
   const_reference operator[](const std::basic_string<charT, Traits, A>& ns) const
   {
      static_assert(sizeof(charT) <= sizeof(char_type), "Failed internal logic");
      if(ns.empty())
         return m_null;
      std::vector<char_type> s;
      for(unsigned i = 0; i < ns.size(); ++i)
         s.insert(s.end(), ns[i]);
      return named_subexpression(&*s.begin(), &*s.begin() + s.size());
   }

   const_reference prefix() const
   {
      if(m_is_singular)
         raise_logic_error();
      return (*this)[-1];
   }

   const_reference suffix() const
   {
      if(m_is_singular)
         raise_logic_error();
      return (*this)[-2];
   }
   const_iterator begin() const
   {
      return (m_subs.size() > 2) ? (m_subs.begin() + 2) : m_subs.end();
   }
   const_iterator end() const
   {
      return m_subs.end();
   }
   // format:
   template <class OutputIterator, class Functor>
   OutputIterator format(OutputIterator out,
                         Functor fmt,
                         match_flag_type flags = format_default) const
   {
      if(m_is_singular)
         raise_logic_error();
      typedef typename BOOST_REGEX_DETAIL_NS::compute_functor_type<Functor, match_results<BidiIterator, Allocator>, OutputIterator>::type F;
      F func(fmt);
      return func(*this, out, flags);
   }
   template <class Functor>
   string_type format(Functor fmt, match_flag_type flags = format_default) const
   {
      if(m_is_singular)
         raise_logic_error();
      std::basic_string<char_type> result;
      BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<char_type> > i(result);

      typedef typename BOOST_REGEX_DETAIL_NS::compute_functor_type<Functor, match_results<BidiIterator, Allocator>, BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<char_type> > >::type F;
      F func(fmt);

      func(*this, i, flags);
      return result;
   }
   // format with locale:
   template <class OutputIterator, class Functor, class RegexT>
   OutputIterator format(OutputIterator out,
                         Functor fmt,
                         match_flag_type flags,
                         const RegexT& re) const
   {
      if(m_is_singular)
         raise_logic_error();
      typedef ::boost::regex_traits_wrapper<typename RegexT::traits_type> traits_type;
      typedef typename BOOST_REGEX_DETAIL_NS::compute_functor_type<Functor, match_results<BidiIterator, Allocator>, OutputIterator, traits_type>::type F;
      F func(fmt);
      return func(*this, out, flags, re.get_traits());
   }
   template <class RegexT, class Functor>
   string_type format(Functor fmt,
                      match_flag_type flags,
                      const RegexT& re) const
   {
      if(m_is_singular)
         raise_logic_error();
      typedef ::boost::regex_traits_wrapper<typename RegexT::traits_type> traits_type;
      std::basic_string<char_type> result;
      BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<char_type> > i(result);

      typedef typename BOOST_REGEX_DETAIL_NS::compute_functor_type<Functor, match_results<BidiIterator, Allocator>, BOOST_REGEX_DETAIL_NS::string_out_iterator<std::basic_string<char_type> >, traits_type >::type F;
      F func(fmt);

      func(*this, i, flags, re.get_traits());
      return result;
   }

   const_reference get_last_closed_paren()const
   {
      if(m_is_singular)
         raise_logic_error();
      return m_last_closed_paren == 0 ? m_null : (*this)[m_last_closed_paren];
   }

   allocator_type get_allocator() const
   {
      return m_subs.get_allocator();
   }
   void swap(match_results& that)
   {
      std::swap(m_subs, that.m_subs);
      std::swap(m_named_subs, that.m_named_subs);
      std::swap(m_last_closed_paren, that.m_last_closed_paren);
      if(m_is_singular)
      {
         if(!that.m_is_singular)
         {
            m_base = that.m_base;
            m_null = that.m_null;
         }
      }
      else if(that.m_is_singular)
      {
         that.m_base = m_base;
         that.m_null = m_null;
      }
      else
      {
         std::swap(m_base, that.m_base);
         std::swap(m_null, that.m_null);
      }
      std::swap(m_is_singular, that.m_is_singular);
   }
   bool operator==(const match_results& that)const
   {
      if(m_is_singular)
      {
         return that.m_is_singular;
      }
      else if(that.m_is_singular)
      {
         return false;
      }
      return (m_subs == that.m_subs) && (m_base == that.m_base) && (m_last_closed_paren == that.m_last_closed_paren);
   }
   bool operator!=(const match_results& that)const
   { return !(*this == that); }

#ifdef BOOST_REGEX_MATCH_EXTRA
   typedef typename sub_match<BidiIterator>::capture_sequence_type capture_sequence_type;

   const capture_sequence_type& captures(int i)const
   {
      if(m_is_singular)
         raise_logic_error();
      return (*this)[i].captures();
   }
#endif

   //
   // private access functions:
   void  set_second(BidiIterator i)
   {
      BOOST_REGEX_ASSERT(m_subs.size() > 2);
      m_subs[2].second = i;
      m_subs[2].matched = true;
      m_subs[0].first = i;
      m_subs[0].matched = (m_subs[0].first != m_subs[0].second);
      m_null.first = i;
      m_null.second = i;
      m_null.matched = false;
      m_is_singular = false;
   }

   void  set_second(BidiIterator i, size_type pos, bool m = true, bool escape_k = false)
   {
      if(pos)
         m_last_closed_paren = static_cast<int>(pos);
      pos += 2;
      BOOST_REGEX_ASSERT(m_subs.size() > pos);
      m_subs[pos].second = i;
      m_subs[pos].matched = m;
      if((pos == 2) && !escape_k)
      {
         m_subs[0].first = i;
         m_subs[0].matched = (m_subs[0].first != m_subs[0].second);
         m_null.first = i;
         m_null.second = i;
         m_null.matched = false;
         m_is_singular = false;
      }
   }
   void  set_size(size_type n, BidiIterator i, BidiIterator j)
   {
      value_type v(j);
      size_type len = m_subs.size();
      if(len > n + 2)
      {
         m_subs.erase(m_subs.begin()+n+2, m_subs.end());
         std::fill(m_subs.begin(), m_subs.end(), v);
      }
      else
      {
         std::fill(m_subs.begin(), m_subs.end(), v);
         if(n+2 != len)
            m_subs.insert(m_subs.end(), n+2-len, v);
      }
      m_subs[1].first = i;
      m_last_closed_paren = 0;
   }
   void  set_base(BidiIterator pos)
   {
      m_base = pos;
   }
   BidiIterator base()const
   {
      return m_base;
   }
   void  set_first(BidiIterator i)
   {
      BOOST_REGEX_ASSERT(m_subs.size() > 2);
      // set up prefix:
      m_subs[1].second = i;
      m_subs[1].matched = (m_subs[1].first != i);
      // set up $0:
      m_subs[2].first = i;
      // zero out everything else:
      for(size_type n = 3; n < m_subs.size(); ++n)
      {
         m_subs[n].first = m_subs[n].second = m_subs[0].second;
         m_subs[n].matched = false;
      }
   }
   void  set_first(BidiIterator i, size_type pos, bool escape_k = false)
   {
      BOOST_REGEX_ASSERT(pos+2 < m_subs.size());
      if(pos || escape_k)
      {
         m_subs[pos+2].first = i;
         if(escape_k)
         {
            m_subs[1].second = i;
            m_subs[1].matched = (m_subs[1].first != m_subs[1].second);
         }
      }
      else
         set_first(i);
   }
   void  maybe_assign(const match_results<BidiIterator, Allocator>& m);

   void  set_named_subs(std::shared_ptr<named_sub_type> subs)
   {
      m_named_subs = subs;
   }

private:
   //
   // Error handler called when an uninitialized match_results is accessed:
   //
   static void raise_logic_error()
   {
      std::logic_error e("Attempt to access an uninitialized boost::match_results<> class.");
#ifndef BOOST_REGEX_STANDALONE
      boost::throw_exception(e);
#else
      throw e;
#endif
   }


   vector_type            m_subs;                      // subexpressions
   BidiIterator   m_base;                              // where the search started from
   sub_match<BidiIterator> m_null;                     // a null match
   std::shared_ptr<named_sub_type> m_named_subs;     // Shared copy of named subs in the regex object
   int m_last_closed_paren;                            // Last ) to be seen - used for formatting
   bool m_is_singular;                                 // True if our stored iterators are singular
};

template <class BidiIterator, class Allocator>
void  match_results<BidiIterator, Allocator>::maybe_assign(const match_results<BidiIterator, Allocator>& m)
{
   if(m_is_singular)
   {
      *this = m;
      return;
   }
   const_iterator p1, p2;
   p1 = begin();
   p2 = m.begin();
   //
   // Distances are measured from the start of *this* match, unless this isn't
   // a valid match in which case we use the start of the whole sequence.  Note that
   // no subsequent match-candidate can ever be to the left of the first match found.
   // This ensures that when we are using bidirectional iterators, that distances 
   // measured are as short as possible, and therefore as efficient as possible
   // to compute.  Finally note that we don't use the "matched" data member to test
   // whether a sub-expression is a valid match, because partial matches set this
   // to false for sub-expression 0.
   //
   BidiIterator l_end = this->suffix().second;
   BidiIterator l_base = (p1->first == l_end) ? this->prefix().first : (*this)[0].first;
   difference_type len1 = 0;
   difference_type len2 = 0;
   difference_type base1 = 0;
   difference_type base2 = 0;
   std::size_t i;
   for(i = 0; i < size(); ++i, ++p1, ++p2)
   {
      //
      // Leftmost takes priority over longest; handle special cases
      // where distances need not be computed first (an optimisation
      // for bidirectional iterators: ensure that we don't accidently
      // compute the length of the whole sequence, as this can be really
      // expensive).
      //
      if(p1->first == l_end)
      {
         if(p2->first != l_end)
         {
            // p2 must be better than p1, and no need to calculate
            // actual distances:
            base1 = 1;
            base2 = 0;
            break;
         }
         else
         {
            // *p1 and *p2 are either unmatched or match end-of sequence,
            // either way no need to calculate distances:
            if((p1->matched == false) && (p2->matched == true))
               break;
            if((p1->matched == true) && (p2->matched == false))
               return;
            continue;
         }
      }
      else if(p2->first == l_end)
      {
         // p1 better than p2, and no need to calculate distances:
         return;
      }
      base1 = std::distance(l_base, p1->first);
      base2 = std::distance(l_base, p2->first);
      BOOST_REGEX_ASSERT(base1 >= 0);
      BOOST_REGEX_ASSERT(base2 >= 0);
      if(base1 < base2) return;
      if(base2 < base1) break;

      len1 = std::distance((BidiIterator)p1->first, (BidiIterator)p1->second);
      len2 = std::distance((BidiIterator)p2->first, (BidiIterator)p2->second);
      BOOST_REGEX_ASSERT(len1 >= 0);
      BOOST_REGEX_ASSERT(len2 >= 0);
      if((len1 != len2) || ((p1->matched == false) && (p2->matched == true)))
         break;
      if((p1->matched == true) && (p2->matched == false))
         return;
   }
   if(i == size())
      return;
   if(base2 < base1)
      *this = m;
   else if((len2 > len1) || ((p1->matched == false) && (p2->matched == true)) )
      *this = m;
}

template <class BidiIterator, class Allocator>
void swap(match_results<BidiIterator, Allocator>& a, match_results<BidiIterator, Allocator>& b)
{
   a.swap(b);
}

template <class charT, class traits, class BidiIterator, class Allocator>
std::basic_ostream<charT, traits>&
   operator << (std::basic_ostream<charT, traits>& os,
                const match_results<BidiIterator, Allocator>& s)
{
   return (os << s.str());
}

#ifdef BOOST_REGEX_MSVC
#pragma warning(pop)
#endif
} // namespace boost

#endif


