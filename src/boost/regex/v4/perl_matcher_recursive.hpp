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
  *                specific to the recursive implementation.
  */

#ifndef BOOST_REGEX_V4_PERL_MATCHER_RECURSIVE_HPP
#define BOOST_REGEX_V4_PERL_MATCHER_RECURSIVE_HPP

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

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4800)
#endif

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

template <class BidiIterator>
class backup_subex
{
   int index;
   sub_match<BidiIterator> sub;
public:
   template <class A>
   backup_subex(const match_results<BidiIterator, A>& w, int i)
      : index(i), sub(w[i], false) {}
   template <class A>
   void restore(match_results<BidiIterator, A>& w)
   {
      w.set_first(sub.first, index, index == 0);
      w.set_second(sub.second, index, sub.matched, index == 0);
   }
   const sub_match<BidiIterator>& get() { return sub; }
};

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_all_states()
{
   static matcher_proc_type const s_match_vtable[34] = 
   {
      (&perl_matcher<BidiIterator, Allocator, traits>::match_startmark),
      &perl_matcher<BidiIterator, Allocator, traits>::match_endmark,
      &perl_matcher<BidiIterator, Allocator, traits>::match_literal,
      &perl_matcher<BidiIterator, Allocator, traits>::match_start_line,
      &perl_matcher<BidiIterator, Allocator, traits>::match_end_line,
      &perl_matcher<BidiIterator, Allocator, traits>::match_wild,
      &perl_matcher<BidiIterator, Allocator, traits>::match_match,
      &perl_matcher<BidiIterator, Allocator, traits>::match_word_boundary,
      &perl_matcher<BidiIterator, Allocator, traits>::match_within_word,
      &perl_matcher<BidiIterator, Allocator, traits>::match_word_start,
      &perl_matcher<BidiIterator, Allocator, traits>::match_word_end,
      &perl_matcher<BidiIterator, Allocator, traits>::match_buffer_start,
      &perl_matcher<BidiIterator, Allocator, traits>::match_buffer_end,
      &perl_matcher<BidiIterator, Allocator, traits>::match_backref,
      &perl_matcher<BidiIterator, Allocator, traits>::match_long_set,
      &perl_matcher<BidiIterator, Allocator, traits>::match_set,
      &perl_matcher<BidiIterator, Allocator, traits>::match_jump,
      &perl_matcher<BidiIterator, Allocator, traits>::match_alt,
      &perl_matcher<BidiIterator, Allocator, traits>::match_rep,
      &perl_matcher<BidiIterator, Allocator, traits>::match_combining,
      &perl_matcher<BidiIterator, Allocator, traits>::match_soft_buffer_end,
      &perl_matcher<BidiIterator, Allocator, traits>::match_restart_continue,
      // Although this next line *should* be evaluated at compile time, in practice
      // some compilers (VC++) emit run-time initialisation which breaks thread
      // safety, so use a dispatch function instead:
      //(::boost::is_random_access_iterator<BidiIterator>::value ? &perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_fast : &perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_slow),
      &perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_dispatch,
      &perl_matcher<BidiIterator, Allocator, traits>::match_char_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::match_set_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::match_long_set_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::match_backstep,
      &perl_matcher<BidiIterator, Allocator, traits>::match_assert_backref,
      &perl_matcher<BidiIterator, Allocator, traits>::match_toggle_case,
      &perl_matcher<BidiIterator, Allocator, traits>::match_recursion,
      &perl_matcher<BidiIterator, Allocator, traits>::match_fail,
      &perl_matcher<BidiIterator, Allocator, traits>::match_accept,
      &perl_matcher<BidiIterator, Allocator, traits>::match_commit,
      &perl_matcher<BidiIterator, Allocator, traits>::match_then,
   };

   if(state_count > max_state_count)
      raise_error(traits_inst, regex_constants::error_complexity);
   while(pstate)
   {
      matcher_proc_type proc = s_match_vtable[pstate->type];
      ++state_count;
      if(!(this->*proc)())
      {
         if((m_match_flags & match_partial) && (position == last) && (position != search_base))
            m_has_partial_match = true;
         return 0;
      }
   }
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_startmark()
{
   int index = static_cast<const re_brace*>(pstate)->index;
   icase = static_cast<const re_brace*>(pstate)->icase;
   bool r = true;
   switch(index)
   {
   case 0:
      pstate = pstate->next.p;
      break;
   case -1:
   case -2:
      {
         // forward lookahead assert:
         BidiIterator old_position(position);
         const re_syntax_base* next_pstate = static_cast<const re_jump*>(pstate->next.p)->alt.p->next.p;
         pstate = pstate->next.p->next.p;
         r = match_all_states();
         pstate = next_pstate;
         position = old_position;
         if((r && (index != -1)) || (!r && (index != -2)))
            r = false;
         else
            r = true;
         if(r && m_have_accept)
            r = skip_until_paren(INT_MAX);
         break;
      }
   case -3:
      {
         // independent sub-expression:
         bool old_independent = m_independent;
         m_independent = true;
         const re_syntax_base* next_pstate = static_cast<const re_jump*>(pstate->next.p)->alt.p->next.p;
         pstate = pstate->next.p->next.p;
         bool can_backtrack = m_can_backtrack;
         r = match_all_states();
         if(r)
            m_can_backtrack = can_backtrack;
         pstate = next_pstate;
         m_independent = old_independent;
#ifdef BOOST_REGEX_MATCH_EXTRA
         if(r && (m_match_flags & match_extra))
         {
            //
            // our captures have been stored in *m_presult
            // we need to unpack them, and insert them
            // back in the right order when we unwind the stack:
            //
            unsigned i;
            match_results<BidiIterator, Allocator> tm(*m_presult);
            for(i = 0; i < tm.size(); ++i)
               (*m_presult)[i].get_captures().clear();
            // match everything else:
            r = match_all_states();
            // now place the stored captures back:
            for(i = 0; i < tm.size(); ++i)
            {
               typedef typename sub_match<BidiIterator>::capture_sequence_type seq;
               seq& s1 = (*m_presult)[i].get_captures();
               const seq& s2 = tm[i].captures();
               s1.insert(
                  s1.end(), 
                  s2.begin(), 
                  s2.end());
            }
         }
#endif
         if(r && m_have_accept)
            r = skip_until_paren(INT_MAX);
         break;
      }
   case -4:
      {
      // conditional expression:
      const re_alt* alt = static_cast<const re_alt*>(pstate->next.p);
      BOOST_REGEX_ASSERT(alt->type == syntax_element_alt);
      pstate = alt->next.p;
      if(pstate->type == syntax_element_assert_backref)
      {
         if(!match_assert_backref())
            pstate = alt->alt.p;
         break;
      }
      else
      {
         // zero width assertion, have to match this recursively:
         BOOST_REGEX_ASSERT(pstate->type == syntax_element_startmark);
         bool negated = static_cast<const re_brace*>(pstate)->index == -2;
         BidiIterator saved_position = position;
         const re_syntax_base* next_pstate = static_cast<const re_jump*>(pstate->next.p)->alt.p->next.p;
         pstate = pstate->next.p->next.p;
         bool res = match_all_states();
         position = saved_position;
         if(negated)
            res = !res;
         if(res)
            pstate = next_pstate;
         else
            pstate = alt->alt.p;
         break;
      }
      }
   case -5:
      {
         // Reset start of $0, since we have a \K escape
         backup_subex<BidiIterator> sub(*m_presult, 0);
         m_presult->set_first(position, 0, true);
         pstate = pstate->next.p;
         r = match_all_states();
         if(r == false)
            sub.restore(*m_presult);
         break;
      }
   default:
   {
      BOOST_REGEX_ASSERT(index > 0);
      if((m_match_flags & match_nosubs) == 0)
      {
         backup_subex<BidiIterator> sub(*m_presult, index);
         m_presult->set_first(position, index);
         pstate = pstate->next.p;
         r = match_all_states();
         if(r == false)
            sub.restore(*m_presult);
#ifdef BOOST_REGEX_MATCH_EXTRA
         //
         // we have a match, push the capture information onto the stack:
         //
         else if(sub.get().matched && (match_extra & m_match_flags))
            ((*m_presult)[index]).get_captures().push_back(sub.get());
#endif
      }
      else
      {
         pstate = pstate->next.p;
      }
      break;
   }
   }
   return r;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_alt()
{
   bool take_first, take_second;
   const re_alt* jmp = static_cast<const re_alt*>(pstate);

   // find out which of these two alternatives we need to take:
   if(position == last)
   {
      take_first = jmp->can_be_null & mask_take;
      take_second = jmp->can_be_null & mask_skip;
   }
   else
   {
      take_first = can_start(*position, jmp->_map, (unsigned char)mask_take);
      take_second = can_start(*position, jmp->_map, (unsigned char)mask_skip);
  }

   if(take_first)
   {
      // we can take the first alternative,
      // see if we need to push next alternative:
      if(take_second)
      {
         BidiIterator oldposition(position);
         const re_syntax_base* old_pstate = jmp->alt.p;
         pstate = pstate->next.p;
         bool oldcase = icase;
         m_have_then = false;
         if(!match_all_states())
         {
            pstate = old_pstate;
            position = oldposition;
            icase = oldcase;
            if(m_have_then)
            {
               m_can_backtrack = true;
               m_have_then = false;
               return false;
            }
         }
         m_have_then = false;
         return m_can_backtrack;
      }
      pstate = pstate->next.p;
      return true;
   }
   if(take_second)
   {
      pstate = jmp->alt.p;
      return true;
   }
   return false;  // neither option is possible
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_rep()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127 4244)
#endif
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   //
   // Always copy the repeat count, so that the state is restored
   // when we exit this scope:
   //
   repeater_count<BidiIterator> r(rep->state_id, &next_count, position, this->recursion_stack.size() ? this->recursion_stack.back().idx : INT_MIN + 3);
   //
   // If we've had at least one repeat already, and the last one 
   // matched the NULL string then set the repeat count to
   // maximum:
   //
   next_count->check_null_repeat(position, rep->max);

   // find out which of these two alternatives we need to take:
   bool take_first, take_second;
   if(position == last)
   {
      take_first = rep->can_be_null & mask_take;
      take_second = rep->can_be_null & mask_skip;
   }
   else
   {
      take_first = can_start(*position, rep->_map, (unsigned char)mask_take);
      take_second = can_start(*position, rep->_map, (unsigned char)mask_skip);
   }

   if(next_count->get_count() < rep->min)
   {
      // we must take the repeat:
      if(take_first)
      {
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         return match_all_states();
      }
      return false;
   }
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   if(greedy)
   {
      // try and take the repeat if we can:
      if((next_count->get_count() < rep->max) && take_first)
      {
         // store position in case we fail:
         BidiIterator pos = position;
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         if(match_all_states())
            return true;
         if(!m_can_backtrack)
            return false;
         // failed repeat, reset posistion and fall through for alternative:
         position = pos;
      }
      if(take_second)
      {
         pstate = rep->alt.p;
         return true;
      }
      return false; // can't take anything, fail...
   }
   else // non-greedy
   {
      // try and skip the repeat if we can:
      if(take_second)
      {
         // store position in case we fail:
         BidiIterator pos = position;
         pstate = rep->alt.p;
         if(match_all_states())
            return true;
         if(!m_can_backtrack)
            return false;
         // failed alternative, reset posistion and fall through for repeat:
         position = pos;
      }
      if((next_count->get_count() < rep->max) && take_first)
      {
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         return match_all_states();
      }
   }
   return false;
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_slow()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   std::size_t count = 0;
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   re_syntax_base* psingle = rep->next.p;
   // match compulsary repeats first:
   while(count < rep->min)
   {
      pstate = psingle;
      if(!match_wild())
         return false;
      ++count;
   }
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   if(greedy)
   {
      // normal repeat:
      while(count < rep->max)
      {
         pstate = psingle;
         if(!match_wild())
            break;
         ++count;
      }
      if((rep->leading) && (count < rep->max))
         restart = position;
      pstate = rep;
      return backtrack_till_match(count - rep->min);
   }
   else
   {
      // non-greedy, keep trying till we get a match:
      BidiIterator save_pos;
      do
      {
         if((rep->leading) && (rep->max == UINT_MAX))
            restart = position;
         pstate = rep->alt.p;
         save_pos = position;
         ++state_count;
         if(match_all_states())
            return true;
         if((count >= rep->max) || !m_can_backtrack)
            return false;
         ++count;
         pstate = psingle;
         position = save_pos;
         if(!match_wild())
            return false;
      }while(true);
   }
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_fast()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   if(m_match_flags & match_not_dot_null)
      return match_dot_repeat_slow();
   if((static_cast<const re_dot*>(pstate->next.p)->mask & match_any_mask) == 0)
      return match_dot_repeat_slow();
   //
   // start by working out how much we can skip:
   //
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4267)
#endif
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   std::size_t count = (std::min)(static_cast<std::size_t>(::boost::BOOST_REGEX_DETAIL_NS::distance(position, last)), greedy ? rep->max : rep->min);
   if(rep->min > count)
   {
      position = last;
      return false;  // not enough text left to match
   }
   std::advance(position, count);
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
   if((rep->leading) && (count < rep->max) && greedy)
      restart = position;
   if(greedy)
      return backtrack_till_match(count - rep->min);

   // non-greedy, keep trying till we get a match:
   BidiIterator save_pos;
   do
   {
      while((position != last) && (count < rep->max) && !can_start(*position, rep->_map, mask_skip))
      {
         ++position;
         ++count;
      }
      if((rep->leading) && (rep->max == UINT_MAX))
         restart = position;
      pstate = rep->alt.p;
      save_pos = position;
      ++state_count;
      if(match_all_states())
         return true;
      if((count >= rep->max) || !m_can_backtrack)
         return false;
      if(save_pos == last)
         return false;
      position = ++save_pos;
      ++count;
   }while(true);
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_char_repeat()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#pragma warning(disable:4267)
#endif
#ifdef BOOST_BORLANDC
#pragma option push -w-8008 -w-8066 -w-8004
#endif
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   BOOST_REGEX_ASSERT(1 == static_cast<const re_literal*>(rep->next.p)->length);
   const char_type what = *reinterpret_cast<const char_type*>(static_cast<const re_literal*>(rep->next.p) + 1);
   //
   // start by working out how much we can skip:
   //
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   std::size_t count, desired;
   if(::boost::is_random_access_iterator<BidiIterator>::value)
   {
      desired = 
         (std::min)(
            (std::size_t)(greedy ? rep->max : rep->min),
            (std::size_t)::boost::BOOST_REGEX_DETAIL_NS::distance(position, last));
      count = desired;
      ++desired;
      if(icase)
      {
         while(--desired && (traits_inst.translate_nocase(*position) == what))
         {
            ++position;
         }
      }
      else
      {
         while(--desired && (traits_inst.translate(*position) == what))
         {
            ++position;
         }
      }
      count = count - desired;
   }
   else
   {
      count = 0;
      desired = greedy ? rep->max : rep->min;
      while((count < desired) && (position != last) && (traits_inst.translate(*position, icase) == what))
      {
         ++position;
         ++count;
      }
   }
   if((rep->leading) && (count < rep->max) && greedy)
      restart = position;
   if(count < rep->min)
      return false;

   if(greedy)
      return backtrack_till_match(count - rep->min);

   // non-greedy, keep trying till we get a match:
   BidiIterator save_pos;
   do
   {
      while((position != last) && (count < rep->max) && !can_start(*position, rep->_map, mask_skip))
      {
         if((traits_inst.translate(*position, icase) == what))
         {
            ++position;
            ++count;
         }
         else
            return false;  // counldn't repeat even though it was the only option
      }
      if((rep->leading) && (rep->max == UINT_MAX))
         restart = position;
      pstate = rep->alt.p;
      save_pos = position;
      ++state_count;
      if(match_all_states())
         return true;
      if((count >= rep->max) || !m_can_backtrack)
         return false;
      position = save_pos;
      if(position == last)
         return false;
      if(traits_inst.translate(*position, icase) == what)
      {
         ++position;
         ++count;
      }
      else
      {
         return false;
      }
   }while(true);
#ifdef BOOST_BORLANDC
#pragma option pop
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_set_repeat()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
#ifdef BOOST_BORLANDC
#pragma option push -w-8008 -w-8066 -w-8004
#endif
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   const unsigned char* map = static_cast<const re_set*>(rep->next.p)->_map;
   std::size_t count = 0;
   //
   // start by working out how much we can skip:
   //
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   std::size_t desired = greedy ? rep->max : rep->min;
   if(::boost::is_random_access_iterator<BidiIterator>::value)
   {
      BidiIterator end = position;
      // Move end forward by "desired", preferably without using distance or advance if we can
      // as these can be slow for some iterator types.
      std::size_t len = (desired == (std::numeric_limits<std::size_t>::max)()) ? 0u : ::boost::BOOST_REGEX_DETAIL_NS::distance(position, last);
      if(desired >= len)
         end = last;
      else
         std::advance(end, desired);
      BidiIterator origin(position);
      while((position != end) && map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
      {
         ++position;
      }
      count = (unsigned)::boost::BOOST_REGEX_DETAIL_NS::distance(origin, position);
   }
   else
   {
      while((count < desired) && (position != last) && map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
      {
         ++position;
         ++count;
      }
   }
   if((rep->leading) && (count < rep->max) && greedy)
      restart = position;
   if(count < rep->min)
      return false;

   if(greedy)
      return backtrack_till_match(count - rep->min);

   // non-greedy, keep trying till we get a match:
   BidiIterator save_pos;
   do
   {
      while((position != last) && (count < rep->max) && !can_start(*position, rep->_map, mask_skip))
      {
         if(map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
         {
            ++position;
            ++count;
         }
         else
            return false;  // counldn't repeat even though it was the only option
      }
      if((rep->leading) && (rep->max == UINT_MAX))
         restart = position;
      pstate = rep->alt.p;
      save_pos = position;
      ++state_count;
      if(match_all_states())
         return true;
      if((count >= rep->max) || !m_can_backtrack)
         return false;
      position = save_pos;
      if(position == last)
         return false;
      if(map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
      {
         ++position;
         ++count;
      }
      else
      {
         return false;
      }
   }while(true);
#ifdef BOOST_BORLANDC
#pragma option pop
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_long_set_repeat()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
#ifdef BOOST_BORLANDC
#pragma option push -w-8008 -w-8066 -w-8004
#endif
   typedef typename traits::char_class_type char_class_type;
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   const re_set_long<char_class_type>* set = static_cast<const re_set_long<char_class_type>*>(pstate->next.p);
   std::size_t count = 0;
   //
   // start by working out how much we can skip:
   //
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   std::size_t desired = greedy ? rep->max : rep->min;
   if(::boost::is_random_access_iterator<BidiIterator>::value)
   {
      BidiIterator end = position;
      // Move end forward by "desired", preferably without using distance or advance if we can
      // as these can be slow for some iterator types.
      std::size_t len = (desired == (std::numeric_limits<std::size_t>::max)()) ? 0u : ::boost::BOOST_REGEX_DETAIL_NS::distance(position, last);
      if(desired >= len)
         end = last;
      else
         std::advance(end, desired);
      BidiIterator origin(position);
      while((position != end) && (position != re_is_set_member(position, last, set, re.get_data(), icase)))
      {
         ++position;
      }
      count = (unsigned)::boost::BOOST_REGEX_DETAIL_NS::distance(origin, position);
   }
   else
   {
      while((count < desired) && (position != last) && (position != re_is_set_member(position, last, set, re.get_data(), icase)))
      {
         ++position;
         ++count;
      }
   }
   if((rep->leading) && (count < rep->max) && greedy)
      restart = position;
   if(count < rep->min)
      return false;

   if(greedy)
      return backtrack_till_match(count - rep->min);

   // non-greedy, keep trying till we get a match:
   BidiIterator save_pos;
   do
   {
      while((position != last) && (count < rep->max) && !can_start(*position, rep->_map, mask_skip))
      {
         if(position != re_is_set_member(position, last, set, re.get_data(), icase))
         {
            ++position;
            ++count;
         }
         else
            return false;  // counldn't repeat even though it was the only option
      }
      if((rep->leading) && (rep->max == UINT_MAX))
         restart = position;
      pstate = rep->alt.p;
      save_pos = position;
      ++state_count;
      if(match_all_states())
         return true;
      if((count >= rep->max) || !m_can_backtrack)
         return false;
      position = save_pos;
      if(position == last)
         return false;
      if(position != re_is_set_member(position, last, set, re.get_data(), icase))
      {
         ++position;
         ++count;
      }
      else
      {
         return false;
      }
   }while(true);
#ifdef BOOST_BORLANDC
#pragma option pop
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::backtrack_till_match(std::size_t count)
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
   if(!m_can_backtrack)
      return false;
   if((m_match_flags & match_partial) && (position == last))
      m_has_partial_match = true;

   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   BidiIterator backtrack = position;
   if(position == last)
   {
      if(rep->can_be_null & mask_skip) 
      {
         pstate = rep->alt.p;
         if(match_all_states())
            return true;
      }
      if(count)
      {
         position = --backtrack;
         --count;
      }
      else
         return false;
   }
   do
   {
      while(count && !can_start(*position, rep->_map, mask_skip))
      {
         --position;
         --count;
         ++state_count;
      }
      pstate = rep->alt.p;
      backtrack = position;
      if(match_all_states())
         return true;
      if(count == 0)
         return false;
      position = --backtrack;
      ++state_count;
      --count;
   }while(true);
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_recursion()
{
   BOOST_REGEX_ASSERT(pstate->type == syntax_element_recurse);
   //
   // Set new call stack:
   //
   if(recursion_stack.capacity() == 0)
   {
      recursion_stack.reserve(50);
   }
   //
   // See if we've seen this recursion before at this location, if we have then
   // we need to prevent infinite recursion:
   //
   for(typename std::vector<recursion_info<results_type> >::reverse_iterator i = recursion_stack.rbegin(); i != recursion_stack.rend(); ++i)
   {
      if(i->idx == static_cast<const re_brace*>(static_cast<const re_jump*>(pstate)->alt.p)->index)
      {
         if(i->location_of_start == position)
            return false;
         break;
      }
   }
   //
   // Now get on with it:
   //
   recursion_stack.push_back(recursion_info<results_type>());
   recursion_stack.back().preturn_address = pstate->next.p;
   recursion_stack.back().results = *m_presult;
   recursion_stack.back().repeater_stack = next_count;
   recursion_stack.back().location_of_start = position;
   pstate = static_cast<const re_jump*>(pstate)->alt.p;
   recursion_stack.back().idx = static_cast<const re_brace*>(pstate)->index;

   repeater_count<BidiIterator>* saved = next_count;
   repeater_count<BidiIterator> r(&next_count); // resets all repeat counts since we're recursing and starting fresh on those
   next_count = &r;
   bool can_backtrack = m_can_backtrack;
   bool result = match_all_states();
   m_can_backtrack = can_backtrack;
   next_count = saved;

   if(!result)
   {
      next_count = recursion_stack.back().repeater_stack;
      *m_presult = recursion_stack.back().results;
      recursion_stack.pop_back();
      return false;
   }
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_endmark()
{
   int index = static_cast<const re_brace*>(pstate)->index;
   icase = static_cast<const re_brace*>(pstate)->icase;
   if(index > 0)
   {
      if((m_match_flags & match_nosubs) == 0)
      {
         m_presult->set_second(position, index);
      }
      if(!recursion_stack.empty())
      {
         if(index == recursion_stack.back().idx)
         {
            recursion_info<results_type> saved = recursion_stack.back();
            recursion_stack.pop_back();
            pstate = saved.preturn_address;
            repeater_count<BidiIterator>* saved_count = next_count;
            next_count = saved.repeater_stack;
            *m_presult = saved.results;
            if(!match_all_states())
            {
               recursion_stack.push_back(saved);
               next_count = saved_count;
               return false;
            }
         }
      }
   }
   else if((index < 0) && (index != -4))
   {
      // matched forward lookahead:
      pstate = 0;
      return true;
   }
   pstate = pstate ? pstate->next.p : 0;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_match()
{
   if(!recursion_stack.empty())
   {
      BOOST_REGEX_ASSERT(0 == recursion_stack.back().idx);
      const re_syntax_base* saved_state = pstate = recursion_stack.back().preturn_address;
      *m_presult = recursion_stack.back().results;
      recursion_stack.pop_back();
      if(!match_all_states())
      {
         recursion_stack.push_back(recursion_info<results_type>());
         recursion_stack.back().preturn_address = saved_state;
         recursion_stack.back().results = *m_presult;
         recursion_stack.back().location_of_start = position;
         return false;
      }
      return true;
   }
   if((m_match_flags & match_not_null) && (position == (*m_presult)[0].first))
      return false;
   if((m_match_flags & match_all) && (position != last))
      return false;
   if((m_match_flags & regex_constants::match_not_initial_null) && (position == search_base))
      return false;
   m_presult->set_second(position);
   pstate = 0;
   m_has_found_match = true;
   if((m_match_flags & match_posix) == match_posix)
   {
      m_result.maybe_assign(*m_presult);
      if((m_match_flags & match_any) == 0)
         return false;
   }
#ifdef BOOST_REGEX_MATCH_EXTRA
   if(match_extra & m_match_flags)
   {
      for(unsigned i = 0; i < m_presult->size(); ++i)
         if((*m_presult)[i].matched)
            ((*m_presult)[i]).get_captures().push_back((*m_presult)[i]);
   }
#endif
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_commit()
{
   m_can_backtrack = false;
   int action = static_cast<const re_commit*>(pstate)->action;
   switch(action)
   {
   case commit_commit:
      restart = last;
      break;
   case commit_skip:
      restart = position;
      break;
   }
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_then()
{
   pstate = pstate->next.p;
   if(match_all_states())
      return true;
   m_can_backtrack = false;
   m_have_then = true;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_toggle_case()
{
   // change our case sensitivity:
   bool oldcase = this->icase;
   this->icase = static_cast<const re_case*>(pstate)->icase;
   pstate = pstate->next.p;
   bool result = match_all_states();
   this->icase = oldcase;
   return result;
}



template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::skip_until_paren(int index, bool have_match)
{
   while(pstate)
   {
      if(pstate->type == syntax_element_endmark)
      {
         if(static_cast<const re_brace*>(pstate)->index == index)
         {
            if(have_match)
               return this->match_endmark();
            pstate = pstate->next.p;
            return true;
         }
         else
         {
            // Unenclosed closing ), occurs when (*ACCEPT) is inside some other 
            // parenthesis which may or may not have other side effects associated with it.
            bool r = match_endmark();
            m_have_accept = true;
            if(!pstate)
               return r;
         }
         continue;
      }
      else if(pstate->type == syntax_element_match)
         return true;
      else if(pstate->type == syntax_element_startmark)
      {
         int idx = static_cast<const re_brace*>(pstate)->index;
         pstate = pstate->next.p;
         skip_until_paren(idx, false);
         continue;
      }
      pstate = pstate->next.p;
   }
   return true;
}


} // namespace BOOST_REGEX_DETAIL_NS
} // namespace boost
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

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

