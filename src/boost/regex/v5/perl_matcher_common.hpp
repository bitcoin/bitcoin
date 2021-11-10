/*
 *
 * Copyright (c) 2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         perl_matcher_common.cpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Definitions of perl_matcher member functions that are 
  *                common to both the recursive and non-recursive versions.
  */

#ifndef BOOST_REGEX_V5_PERL_MATCHER_COMMON_HPP
#define BOOST_REGEX_V5_PERL_MATCHER_COMMON_HPP

#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#pragma warning(disable:4459)
#if BOOST_REGEX_MSVC < 1910
#pragma warning(disable:4800)
#endif
#endif

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

#ifdef BOOST_REGEX_MSVC
#  pragma warning(push)
#pragma warning(disable:26812)
#endif
   template <class BidiIterator, class Allocator, class traits>
void perl_matcher<BidiIterator, Allocator, traits>::construct_init(const basic_regex<char_type, traits>& e, match_flag_type f)
{ 
   typedef typename std::iterator_traits<BidiIterator>::iterator_category category;
   typedef typename basic_regex<char_type, traits>::flag_type expression_flag_type;
   
   if(e.empty())
   {
      // precondition failure: e is not a valid regex.
      std::invalid_argument ex("Invalid regular expression object");
#ifndef BOOST_REGEX_STANDALONE
      boost::throw_exception(ex);
#else
      throw e;
#endif
   }
   pstate = 0;
   m_match_flags = f;
   estimate_max_state_count(static_cast<category*>(0));
   expression_flag_type re_f = re.flags();
   icase = re_f & regex_constants::icase;
   if(!(m_match_flags & (match_perl|match_posix)))
   {
      if((re_f & (regbase::main_option_type|regbase::no_perl_ex)) == 0)
         m_match_flags |= match_perl;
      else if((re_f & (regbase::main_option_type|regbase::emacs_ex)) == (regbase::basic_syntax_group|regbase::emacs_ex))
         m_match_flags |= match_perl;
      else if((re_f & (regbase::main_option_type|regbase::literal)) == (regbase::literal))
         m_match_flags |= match_perl;
      else
         m_match_flags |= match_posix;
   }
   if(m_match_flags & match_posix)
   {
      m_temp_match.reset(new match_results<BidiIterator, Allocator>());
      m_presult = m_temp_match.get();
   }
   else
      m_presult = &m_result;
   m_stack_base = 0;
   m_backup_state = 0;
   // find the value to use for matching word boundaries:
   m_word_mask = re.get_data().m_word_mask; 
   // find bitmask to use for matching '.':
   match_any_mask = static_cast<unsigned char>((f & match_not_dot_newline) ? BOOST_REGEX_DETAIL_NS::test_not_newline : BOOST_REGEX_DETAIL_NS::test_newline);
   // Disable match_any if requested in the state machine:
   if(e.get_data().m_disable_match_any)
      m_match_flags &= regex_constants::match_not_any;
}
#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif

template <class BidiIterator, class Allocator, class traits>
void perl_matcher<BidiIterator, Allocator, traits>::estimate_max_state_count(std::random_access_iterator_tag*)
{
   //
   // How many states should we allow our machine to visit before giving up?
   // This is a heuristic: it takes the greater of O(N^2) and O(NS^2)
   // where N is the length of the string, and S is the number of states
   // in the machine.  It's tempting to up this to O(N^2S) or even O(N^2S^2)
   // but these take unreasonably amounts of time to bale out in pathological
   // cases.
   //
   // Calculate NS^2 first:
   //
   static const std::ptrdiff_t k = 100000;
   std::ptrdiff_t dist = std::distance(base, last);
   if(dist == 0)
      dist = 1;
   std::ptrdiff_t states = re.size();
   if(states == 0)
      states = 1;
   if ((std::numeric_limits<std::ptrdiff_t>::max)() / states < states)
   {
      max_state_count = (std::min)((std::ptrdiff_t)BOOST_REGEX_MAX_STATE_COUNT, (std::numeric_limits<std::ptrdiff_t>::max)() - 2);
      return;
   }
   states *= states;
   if((std::numeric_limits<std::ptrdiff_t>::max)() / dist < states)
   {
      max_state_count = (std::min)((std::ptrdiff_t)BOOST_REGEX_MAX_STATE_COUNT, (std::numeric_limits<std::ptrdiff_t>::max)() - 2);
      return;
   }
   states *= dist;
   if((std::numeric_limits<std::ptrdiff_t>::max)() - k < states)
   {
      max_state_count = (std::min)((std::ptrdiff_t)BOOST_REGEX_MAX_STATE_COUNT, (std::numeric_limits<std::ptrdiff_t>::max)() - 2);
      return;
   }
   states += k;

   max_state_count = states;

   //
   // Now calculate N^2:
   //
   states = dist;
   if((std::numeric_limits<std::ptrdiff_t>::max)() / dist < states)
   {
      max_state_count = (std::min)((std::ptrdiff_t)BOOST_REGEX_MAX_STATE_COUNT, (std::numeric_limits<std::ptrdiff_t>::max)() - 2);
      return;
   }
   states *= dist;
   if((std::numeric_limits<std::ptrdiff_t>::max)() - k < states)
   {
      max_state_count = (std::min)((std::ptrdiff_t)BOOST_REGEX_MAX_STATE_COUNT, (std::numeric_limits<std::ptrdiff_t>::max)() - 2);
      return;
   }
   states += k;
   //
   // N^2 can be a very large number indeed, to prevent things getting out
   // of control, cap the max states:
   //
   if(states > BOOST_REGEX_MAX_STATE_COUNT)
      states = BOOST_REGEX_MAX_STATE_COUNT;
   //
   // If (the possibly capped) N^2 is larger than our first estimate,
   // use this instead:
   //
   if(states > max_state_count)
      max_state_count = states;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::estimate_max_state_count(void*)
{
   // we don't know how long the sequence is:
   max_state_count = BOOST_REGEX_MAX_STATE_COUNT;
}

template <class BidiIterator, class Allocator, class traits>
inline bool perl_matcher<BidiIterator, Allocator, traits>::match()
{
   return match_imp();
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_imp()
{
   // initialise our stack if we are non-recursive:
   save_state_init init(&m_stack_base, &m_backup_state);
   used_block_count = BOOST_REGEX_MAX_BLOCKS;
#if !defined(BOOST_NO_EXCEPTIONS)
   try{
#endif

   // reset our state machine:
   position = base;
   search_base = base;
   state_count = 0;
   m_match_flags |= regex_constants::match_all;
   m_presult->set_size((m_match_flags & match_nosubs) ? 1u : static_cast<typename results_type::size_type>(1u + re.mark_count()), search_base, last);
   m_presult->set_base(base);
   m_presult->set_named_subs(this->re.get_named_subs());
   if(m_match_flags & match_posix)
      m_result = *m_presult;
   verify_options(re.flags(), m_match_flags);
   if(0 == match_prefix())
      return false;
   return (m_result[0].second == last) && (m_result[0].first == base);

#if !defined(BOOST_NO_EXCEPTIONS)
   }
   catch(...)
   {
      // unwind all pushed states, apart from anything else this
      // ensures that all the states are correctly destructed
      // not just the memory freed.
      while(unwind(true)){}
      throw;
   }
#endif
}

template <class BidiIterator, class Allocator, class traits>
inline bool perl_matcher<BidiIterator, Allocator, traits>::find()
{
   return find_imp();
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_imp()
{
   static matcher_proc_type const s_find_vtable[7] = 
   {
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_any,
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_word,
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_line,
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_buf,
      &perl_matcher<BidiIterator, Allocator, traits>::match_prefix,
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_lit,
      &perl_matcher<BidiIterator, Allocator, traits>::find_restart_lit,
   };

   // initialise our stack if we are non-recursive:
   save_state_init init(&m_stack_base, &m_backup_state);
   used_block_count = BOOST_REGEX_MAX_BLOCKS;
#if !defined(BOOST_NO_EXCEPTIONS)
   try{
#endif

   state_count = 0;
   if((m_match_flags & regex_constants::match_init) == 0)
   {
      // reset our state machine:
      search_base = position = base;
      pstate = re.get_first_state();
      m_presult->set_size((m_match_flags & match_nosubs) ? 1u : static_cast<typename results_type::size_type>(1u + re.mark_count()), base, last);
      m_presult->set_base(base);
      m_presult->set_named_subs(this->re.get_named_subs());
      m_match_flags |= regex_constants::match_init;
   }
   else
   {
      // start again:
      search_base = position = m_result[0].second;
      // If last match was null and match_not_null was not set then increment
      // our start position, otherwise we go into an infinite loop:
      if(((m_match_flags & match_not_null) == 0) && (m_result.length() == 0))
      {
         if(position == last)
            return false;
         else 
            ++position;
      }
      // reset $` start:
      m_presult->set_size((m_match_flags & match_nosubs) ? 1u : static_cast<typename results_type::size_type>(1u + re.mark_count()), search_base, last);
      //if((base != search_base) && (base == backstop))
      //   m_match_flags |= match_prev_avail;
   }
   if(m_match_flags & match_posix)
   {
      m_result.set_size(static_cast<typename results_type::size_type>(1u + re.mark_count()), base, last);
      m_result.set_base(base);
   }

   verify_options(re.flags(), m_match_flags);
   // find out what kind of expression we have:
   unsigned type = (m_match_flags & match_continuous) ? 
      static_cast<unsigned int>(regbase::restart_continue) 
         : static_cast<unsigned int>(re.get_restart_type());

   // call the appropriate search routine:
   matcher_proc_type proc = s_find_vtable[type];
   return (this->*proc)();

#if !defined(BOOST_NO_EXCEPTIONS)
   }
   catch(...)
   {
      // unwind all pushed states, apart from anything else this
      // ensures that all the states are correctly destructed
      // not just the memory freed.
      while(unwind(true)){}
      throw;
   }
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_prefix()
{
   m_has_partial_match = false;
   m_has_found_match = false;
   pstate = re.get_first_state();
   m_presult->set_first(position);
   restart = position;
   match_all_states();
   if(!m_has_found_match && m_has_partial_match && (m_match_flags & match_partial))
   {
      m_has_found_match = true;
      m_presult->set_second(last, 0, false);
      position = last;
      if((m_match_flags & match_posix) == match_posix)
      {
         m_result.maybe_assign(*m_presult);
      }
   }
#ifdef BOOST_REGEX_MATCH_EXTRA
   if(m_has_found_match && (match_extra & m_match_flags))
   {
      //
      // we have a match, reverse the capture information:
      //
      for(unsigned i = 0; i < m_presult->size(); ++i)
      {
         typename sub_match<BidiIterator>::capture_sequence_type & seq = ((*m_presult)[i]).get_captures();
         std::reverse(seq.begin(), seq.end());
      }
   }
#endif
   if(!m_has_found_match)
      position = restart; // reset search postion
   return m_has_found_match;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_literal()
{
   unsigned int len = static_cast<const re_literal*>(pstate)->length;
   const char_type* what = reinterpret_cast<const char_type*>(static_cast<const re_literal*>(pstate) + 1);
   //
   // compare string with what we stored in
   // our records:
   for(unsigned int i = 0; i < len; ++i, ++position)
   {
      if((position == last) || (traits_inst.translate(*position, icase) != what[i]))
         return false;
   }
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_start_line()
{
   if(position == backstop)
   {
      if((m_match_flags & match_prev_avail) == 0)
      {
         if((m_match_flags & match_not_bol) == 0)
         {
            pstate = pstate->next.p;
            return true;
         }
         return false;
      }
   }
   else if(m_match_flags & match_single_line)
      return false;

   // check the previous value character:
   BidiIterator t(position);
   --t;
   if(position != last)
   {
      if(is_separator(*t) && !((*t == static_cast<char_type>('\r')) && (*position == static_cast<char_type>('\n'))) )
      {
         pstate = pstate->next.p;
         return true;
      }
   }
   else if(is_separator(*t))
   {
      pstate = pstate->next.p;
      return true;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_end_line()
{
   if(position != last)
   {
      if(m_match_flags & match_single_line)
         return false;
      // we're not yet at the end so *first is always valid:
      if(is_separator(*position))
      {
         if((position != backstop) || (m_match_flags & match_prev_avail))
         {
            // check that we're not in the middle of \r\n sequence
            BidiIterator t(position);
            --t;
            if((*t == static_cast<char_type>('\r')) && (*position == static_cast<char_type>('\n')))
            {
               return false;
            }
         }
         pstate = pstate->next.p;
         return true;
      }
   }
   else if((m_match_flags & match_not_eol) == 0)
   {
      pstate = pstate->next.p;
      return true;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_wild()
{
   if(position == last) 
      return false;
   if(is_separator(*position) && ((match_any_mask & static_cast<const re_dot*>(pstate)->mask) == 0))
      return false;
   if((*position == char_type(0)) && (m_match_flags & match_not_dot_null))
      return false;
   pstate = pstate->next.p;
   ++position;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_word_boundary()
{
   bool b; // indcates whether next character is a word character
   if(position != last)
   {
      // prev and this character must be opposites:
      b = traits_inst.isctype(*position, m_word_mask);
   }
   else
   {
      if (m_match_flags & match_not_eow)
         return false;
      b = false;
   }
   if((position == backstop) && ((m_match_flags & match_prev_avail) == 0))
   {
      if(m_match_flags & match_not_bow)
         return false;
      else
         b ^= false;
   }
   else
   {
      --position;
      b ^= traits_inst.isctype(*position, m_word_mask);
      ++position;
   }
   if(b)
   {
      pstate = pstate->next.p;
      return true;
   }
   return false; // no match if we get to here...
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_within_word()
{
   if(position == last)
      return false;
   // both prev and this character must be m_word_mask:
   bool prev = traits_inst.isctype(*position, m_word_mask);
   {
      bool b;
      if((position == backstop) && ((m_match_flags & match_prev_avail) == 0)) 
         return false;
      else
      {
         --position;
         b = traits_inst.isctype(*position, m_word_mask);
         ++position;
      }
      if(b == prev)
      {
         pstate = pstate->next.p;
         return true;
      }
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_word_start()
{
   if(position == last)
      return false; // can't be starting a word if we're already at the end of input
   if(!traits_inst.isctype(*position, m_word_mask))
      return false; // next character isn't a word character
   if((position == backstop) && ((m_match_flags & match_prev_avail) == 0))
   {
      if(m_match_flags & match_not_bow)
         return false; // no previous input
   }
   else
   {
      // otherwise inside buffer:
      BidiIterator t(position);
      --t;
      if(traits_inst.isctype(*t, m_word_mask))
         return false; // previous character not non-word
   }
   // OK we have a match:
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_word_end()
{
   if((position == backstop) && ((m_match_flags & match_prev_avail) == 0))
      return false;  // start of buffer can't be end of word
   BidiIterator t(position);
   --t;
   if(traits_inst.isctype(*t, m_word_mask) == false)
      return false;  // previous character wasn't a word character

   if(position == last)
   {
      if(m_match_flags & match_not_eow)
         return false; // end of buffer but not end of word
   }
   else
   {
      // otherwise inside buffer:
      if(traits_inst.isctype(*position, m_word_mask))
         return false; // next character is a word character
   }
   pstate = pstate->next.p;
   return true;      // if we fall through to here then we've succeeded
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_buffer_start()
{
   if((position != backstop) || (m_match_flags & match_not_bob))
      return false;
   // OK match:
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_buffer_end()
{
   if((position != last) || (m_match_flags & match_not_eob))
      return false;
   // OK match:
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_backref()
{
   //
   // Compare with what we previously matched.
   // Note that this succeeds if the backref did not partisipate
   // in the match, this is in line with ECMAScript, but not Perl
   // or PCRE.
   //
   int index = static_cast<const re_brace*>(pstate)->index;
   if(index >= hash_value_mask)
   {
      named_subexpressions::range_type r = re.get_data().equal_range(index);
      BOOST_REGEX_ASSERT(r.first != r.second);
      do
      {
         index = r.first->index;
         ++r.first;
      }while((r.first != r.second) && ((*m_presult)[index].matched != true));
   }

   if((m_match_flags & match_perl) && !(*m_presult)[index].matched)
      return false;

   BidiIterator i = (*m_presult)[index].first;
   BidiIterator j = (*m_presult)[index].second;
   while(i != j)
   {
      if((position == last) || (traits_inst.translate(*position, icase) != traits_inst.translate(*i, icase)))
         return false;
      ++i;
      ++position;
   }
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_long_set()
{
   typedef typename traits::char_class_type char_class_type;
   // let the traits class do the work:
   if(position == last)
      return false;
   BidiIterator t = re_is_set_member(position, last, static_cast<const re_set_long<char_class_type>*>(pstate), re.get_data(), icase);
   if(t != position)
   {
      pstate = pstate->next.p;
      position = t;
      return true;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_set()
{
   if(position == last)
      return false;
   if(static_cast<const re_set*>(pstate)->_map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
   {
      pstate = pstate->next.p;
      ++position;
      return true;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_jump()
{
   pstate = static_cast<const re_jump*>(pstate)->alt.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_combining()
{
   if(position == last)
      return false;
   if(is_combining(traits_inst.translate(*position, icase)))
      return false;
   ++position;
   while((position != last) && is_combining(traits_inst.translate(*position, icase)))
      ++position;
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_soft_buffer_end()
{
   if(m_match_flags & match_not_eob)
      return false;
   BidiIterator p(position);
   while((p != last) && is_separator(traits_inst.translate(*p, icase)))++p;
   if(p != last)
      return false;
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_restart_continue()
{
   if(position == search_base)
   {
      pstate = pstate->next.p;
      return true;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_backstep()
{
#ifdef BOOST_REGEX_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   if( ::boost::is_random_access_iterator<BidiIterator>::value)
   {
      std::ptrdiff_t maxlen = std::distance(backstop, position);
      if(maxlen < static_cast<const re_brace*>(pstate)->index)
         return false;
      std::advance(position, -static_cast<const re_brace*>(pstate)->index);
   }
   else
   {
      int c = static_cast<const re_brace*>(pstate)->index;
      while(c--)
      {
         if(position == backstop)
            return false;
         --position;
      }
   }
   pstate = pstate->next.p;
   return true;
#ifdef BOOST_REGEX_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
inline bool perl_matcher<BidiIterator, Allocator, traits>::match_assert_backref()
{
   // return true if marked sub-expression N has been matched:
   int index = static_cast<const re_brace*>(pstate)->index;
   bool result = false;
   if(index == 9999)
   {
      // Magic value for a (DEFINE) block:
      return false;
   }
   else if(index > 0)
   {
      // Have we matched subexpression "index"?
      // Check if index is a hash value:
      if(index >= hash_value_mask)
      {
         named_subexpressions::range_type r = re.get_data().equal_range(index);
         while(r.first != r.second)
         {
            if((*m_presult)[r.first->index].matched)
            {
               result = true;
               break;
            }
            ++r.first;
         }
      }
      else
      {
         result = (*m_presult)[index].matched;
      }
      pstate = pstate->next.p;
   }
   else
   {
      // Have we recursed into subexpression "index"?
      // If index == 0 then check for any recursion at all, otherwise for recursion to -index-1.
      int idx = -(index+1);
      if(idx >= hash_value_mask)
      {
         named_subexpressions::range_type r = re.get_data().equal_range(idx);
         int stack_index = recursion_stack.empty() ? -1 : recursion_stack.back().idx;
         while(r.first != r.second)
         {
            result |= (stack_index == r.first->index);
            if(result)break;
            ++r.first;
         }
      }
      else
      {
         result = !recursion_stack.empty() && ((recursion_stack.back().idx == idx) || (index == 0));
      }
      pstate = pstate->next.p;
   }
   return result;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_fail()
{
   // Just force a backtrack:
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_accept()
{
   if(!recursion_stack.empty())
   {
      return skip_until_paren(recursion_stack.back().idx);
   }
   else
   {
      return skip_until_paren(INT_MAX);
   }
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_restart_any()
{
#ifdef BOOST_REGEX_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   const unsigned char* _map = re.get_map();
   while(true)
   {
      // skip everything we can't match:
      while((position != last) && !can_start(*position, _map, (unsigned char)mask_any) )
         ++position;
      if(position == last)
      {
         // run out of characters, try a null match if possible:
         if(re.can_be_null())
            return match_prefix();
         break;
      }
      // now try and obtain a match:
      if(match_prefix())
         return true;
      if(position == last)
         return false;
      ++position;
   }
   return false;
#ifdef BOOST_REGEX_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_restart_word()
{
#ifdef BOOST_REGEX_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   // do search optimised for word starts:
   const unsigned char* _map = re.get_map();
   if((m_match_flags & match_prev_avail) || (position != base))
      --position;
   else if(match_prefix())
      return true;
   do
   {
      while((position != last) && traits_inst.isctype(*position, m_word_mask))
         ++position;
      while((position != last) && !traits_inst.isctype(*position, m_word_mask))
         ++position;
      if(position == last)
         break;

      if(can_start(*position, _map, (unsigned char)mask_any) )
      {
         if(match_prefix())
            return true;
      }
      if(position == last)
         break;
   } while(true);
   return false;
#ifdef BOOST_REGEX_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_restart_line()
{
   // do search optimised for line starts:
   const unsigned char* _map = re.get_map();
   if(match_prefix())
      return true;
   while(position != last)
   {
      while((position != last) && !is_separator(*position))
         ++position;
      if(position == last)
         return false;
      ++position;
      if(position == last)
      {
         if(re.can_be_null() && match_prefix())
            return true;
         return false;
      }

      if( can_start(*position, _map, (unsigned char)mask_any) )
      {
         if(match_prefix())
            return true;
      }
      if(position == last)
         return false;
      //++position;
   }
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_restart_buf()
{
   if((position == base) && ((m_match_flags & match_not_bob) == 0))
      return match_prefix();
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::find_restart_lit()
{
   return false;
}

} // namespace BOOST_REGEX_DETAIL_NS

} // namespace boost

#ifdef BOOST_REGEX_MSVC
#  pragma warning(pop)
#endif

#endif

