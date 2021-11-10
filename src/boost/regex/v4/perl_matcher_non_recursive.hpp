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
  *                specific to the non-recursive implementation.
  */

#ifndef BOOST_REGEX_V4_PERL_MATCHER_NON_RECURSIVE_HPP
#define BOOST_REGEX_V4_PERL_MATCHER_NON_RECURSIVE_HPP

#include <boost/regex/v4/mem_block_cache.hpp>

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
#  pragma warning(push)
#  pragma warning(disable: 4706)
#if BOOST_MSVC < 1910
#pragma warning(disable:4800)
#endif
#endif

namespace boost{
namespace BOOST_REGEX_DETAIL_NS{

template <class T>
inline void inplace_destroy(T* p)
{
   (void)p;  // warning suppression
   p->~T();
}

struct saved_state
{
   union{
      unsigned int state_id;
      // this padding ensures correct alignment on 64-bit platforms:
      std::size_t padding1;
      std::ptrdiff_t padding2;
      void* padding3;
   };
   saved_state(unsigned i) : state_id(i) {}
};

template <class BidiIterator>
struct saved_matched_paren : public saved_state
{
   int index;
   sub_match<BidiIterator> sub;
   saved_matched_paren(int i, const sub_match<BidiIterator>& s) : saved_state(1), index(i), sub(s){}
};

template <class BidiIterator>
struct saved_position : public saved_state
{
   const re_syntax_base* pstate;
   BidiIterator position;
   saved_position(const re_syntax_base* ps, BidiIterator pos, int i) : saved_state(i), pstate(ps), position(pos){}
};

template <class BidiIterator>
struct saved_assertion : public saved_position<BidiIterator>
{
   bool positive;
   saved_assertion(bool p, const re_syntax_base* ps, BidiIterator pos) 
      : saved_position<BidiIterator>(ps, pos, saved_type_assertion), positive(p){}
};

template <class BidiIterator>
struct saved_repeater : public saved_state
{
   repeater_count<BidiIterator> count;
   saved_repeater(int i, repeater_count<BidiIterator>** s, BidiIterator start, int current_recursion_id)
      : saved_state(saved_state_repeater_count), count(i, s, start, current_recursion_id){}
};

struct saved_extra_block : public saved_state
{
   saved_state *base, *end;
   saved_extra_block(saved_state* b, saved_state* e) 
      : saved_state(saved_state_extra_block), base(b), end(e) {}
};

struct save_state_init
{
   saved_state** stack;
   save_state_init(saved_state** base, saved_state** end)
      : stack(base)
   {
      *base = static_cast<saved_state*>(get_mem_block());
      *end = reinterpret_cast<saved_state*>(reinterpret_cast<char*>(*base)+BOOST_REGEX_BLOCKSIZE);
      --(*end);
      (void) new (*end)saved_state(0);
      BOOST_REGEX_ASSERT(*end > *base);
   }
   ~save_state_init()
   {
      put_mem_block(*stack);
      *stack = 0;
   }
};

template <class BidiIterator>
struct saved_single_repeat : public saved_state
{
   std::size_t count;
   const re_repeat* rep;
   BidiIterator last_position;
   saved_single_repeat(std::size_t c, const re_repeat* r, BidiIterator lp, int arg_id) 
      : saved_state(arg_id), count(c), rep(r), last_position(lp){}
};

template <class Results>
struct saved_recursion : public saved_state
{
   saved_recursion(int idx, const re_syntax_base* p, Results* pr, Results* pr2) 
      : saved_state(14), recursion_id(idx), preturn_address(p), internal_results(*pr), prior_results(*pr2) {}
   int recursion_id;
   const re_syntax_base* preturn_address;
   Results internal_results, prior_results;
};

struct saved_change_case : public saved_state
{
   bool icase;
   saved_change_case(bool c) : saved_state(18), icase(c) {}
};

struct incrementer
{
   incrementer(unsigned* pu) : m_pu(pu) { ++*m_pu; }
   ~incrementer() { --*m_pu; }
   bool operator > (unsigned i) { return *m_pu > i; }
private:
   unsigned* m_pu;
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
   incrementer inc(&m_recursions);
   if(inc > 80)
      raise_error(traits_inst, regex_constants::error_complexity);
   push_recursion_stopper();
   do{
      while(pstate)
      {
         matcher_proc_type proc = s_match_vtable[pstate->type];
         ++state_count;
         if(!(this->*proc)())
         {
            if(state_count > max_state_count)
               raise_error(traits_inst, regex_constants::error_complexity);
            if((m_match_flags & match_partial) && (position == last) && (position != search_base))
               m_has_partial_match = true;
            bool successful_unwind = unwind(false);
            if((m_match_flags & match_partial) && (position == last) && (position != search_base))
               m_has_partial_match = true;
            if(!successful_unwind)
               return m_recursive_result;
         }
      }
   }while(unwind(true));
   return m_recursive_result;
}

template <class BidiIterator, class Allocator, class traits>
void perl_matcher<BidiIterator, Allocator, traits>::extend_stack()
{
   if(used_block_count)
   {
      --used_block_count;
      saved_state* stack_base;
      saved_state* backup_state;
      stack_base = static_cast<saved_state*>(get_mem_block());
      backup_state = reinterpret_cast<saved_state*>(reinterpret_cast<char*>(stack_base)+BOOST_REGEX_BLOCKSIZE);
      saved_extra_block* block = static_cast<saved_extra_block*>(backup_state);
      --block;
      (void) new (block) saved_extra_block(m_stack_base, m_backup_state);
      m_stack_base = stack_base;
      m_backup_state = block;
   }
   else
      raise_error(traits_inst, regex_constants::error_stack);
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_matched_paren(int index, const sub_match<BidiIterator>& sub)
{
   //BOOST_REGEX_ASSERT(index);
   saved_matched_paren<BidiIterator>* pmp = static_cast<saved_matched_paren<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_matched_paren<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_matched_paren<BidiIterator>(index, sub);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_case_change(bool c)
{
   //BOOST_REGEX_ASSERT(index);
   saved_change_case* pmp = static_cast<saved_change_case*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_change_case*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_change_case(c);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_recursion_stopper()
{
   saved_state* pmp = m_backup_state;
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = m_backup_state;
      --pmp;
   }
   (void) new (pmp)saved_state(saved_type_recurse);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_assertion(const re_syntax_base* ps, bool positive)
{
   saved_assertion<BidiIterator>* pmp = static_cast<saved_assertion<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_assertion<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_assertion<BidiIterator>(positive, ps, position);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_alt(const re_syntax_base* ps)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_position<BidiIterator>(ps, position, saved_state_alt);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_non_greedy_repeat(const re_syntax_base* ps)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_position<BidiIterator>(ps, position, saved_state_non_greedy_long_repeat);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_repeater_count(int i, repeater_count<BidiIterator>** s)
{
   saved_repeater<BidiIterator>* pmp = static_cast<saved_repeater<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_repeater<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_repeater<BidiIterator>(i, s, position, this->recursion_stack.empty() ? (INT_MIN + 3) : this->recursion_stack.back().idx);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_single_repeat(std::size_t c, const re_repeat* r, BidiIterator last_position, int state_id)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_single_repeat<BidiIterator>(c, r, last_position, state_id);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_recursion(int idx, const re_syntax_base* p, results_type* presults, results_type* presults2)
{
   saved_recursion<results_type>* pmp = static_cast<saved_recursion<results_type>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_recursion<results_type>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_recursion<results_type>(idx, p, presults, presults2);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_toggle_case()
{
   // change our case sensitivity:
   push_case_change(this->icase);
   this->icase = static_cast<const re_case*>(pstate)->icase;
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_startmark()
{
   int index = static_cast<const re_brace*>(pstate)->index;
   icase = static_cast<const re_brace*>(pstate)->icase;
   switch(index)
   {
   case 0:
      pstate = pstate->next.p;
      break;
   case -1:
   case -2:
      {
         // forward lookahead assert:
         const re_syntax_base* next_pstate = static_cast<const re_jump*>(pstate->next.p)->alt.p->next.p;
         pstate = pstate->next.p->next.p;
         push_assertion(next_pstate, index == -1);
         break;
      }
   case -3:
      {
         // independent sub-expression, currently this is always recursive:
         bool old_independent = m_independent;
         m_independent = true;
         const re_syntax_base* next_pstate = static_cast<const re_jump*>(pstate->next.p)->alt.p->next.p;
         pstate = pstate->next.p->next.p;
         bool r = false;
#if !defined(BOOST_NO_EXCEPTIONS)
      try{
#endif
         r = match_all_states();
         if(!r && !m_independent)
         {
            // Must be unwinding from a COMMIT/SKIP/PRUNE and the independent 
            // sub failed, need to unwind everything else:
            while(unwind(false));
            return false;
         }
#if !defined(BOOST_NO_EXCEPTIONS)
      }
      catch(...)
      {
         pstate = next_pstate;
         // unwind all pushed states, apart from anything else this
         // ensures that all the states are correctly destructed
         // not just the memory freed.
         while(unwind(true)) {}
         throw;
      }
#endif
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
            match_results<BidiIterator, Allocator> temp_match(*m_presult);
            unsigned i;
            for(i = 0; i < temp_match.size(); ++i)
               (*m_presult)[i].get_captures().clear();
            // match everything else:
#if !defined(BOOST_NO_EXCEPTIONS)
            try{
#endif
               r = match_all_states();
#if !defined(BOOST_NO_EXCEPTIONS)
            }
            catch(...)
            {
               pstate = next_pstate;
               // unwind all pushed states, apart from anything else this
               // ensures that all the states are correctly destructed
               // not just the memory freed.
               while(unwind(true)) {}
               throw;
            }
#endif
         // now place the stored captures back:
            for(i = 0; i < temp_match.size(); ++i)
            {
               typedef typename sub_match<BidiIterator>::capture_sequence_type seq;
               seq& s1 = (*m_presult)[i].get_captures();
               const seq& s2 = temp_match[i].captures();
               s1.insert(
                  s1.end(), 
                  s2.begin(), 
                  s2.end());
            }
         }
#endif
         return r;
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
#if !defined(BOOST_NO_EXCEPTIONS)
         try{
#endif
            bool r = match_all_states();
            position = saved_position;
            if(negated)
               r = !r;
            if(r)
               pstate = next_pstate;
            else
               pstate = alt->alt.p;
#if !defined(BOOST_NO_EXCEPTIONS)
         }
         catch(...)
         {
            pstate = next_pstate;
            // unwind all pushed states, apart from anything else this
            // ensures that all the states are correctly destructed
            // not just the memory freed.
            while(unwind(true)){}
            throw;
         }
#endif
         break;
      }
      }
   case -5:
      {
         push_matched_paren(0, (*m_presult)[0]);
         m_presult->set_first(position, 0, true);
         pstate = pstate->next.p;
         break;
      }
   default:
   {
      BOOST_REGEX_ASSERT(index > 0);
      if((m_match_flags & match_nosubs) == 0)
      {
         push_matched_paren(index, (*m_presult)[index]);
         m_presult->set_first(position, index);
      }
      pstate = pstate->next.p;
      break;
   }
   }
   return true;
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
         push_alt(jmp->alt.p);
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
#ifdef BOOST_BORLANDC
#pragma option push -w-8008 -w-8066 -w-8004
#endif
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);

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

   if((m_backup_state->state_id != saved_state_repeater_count) 
      || (static_cast<saved_repeater<BidiIterator>*>(m_backup_state)->count.get_id() != rep->state_id)
      || (next_count->get_id() != rep->state_id))
   {
      // we're moving to a different repeat from the last
      // one, so set up a counter object:
      push_repeater_count(rep->state_id, &next_count);
   }
   //
   // If we've had at least one repeat already, and the last one 
   // matched the NULL string then set the repeat count to
   // maximum:
   //
   next_count->check_null_repeat(position, rep->max);

   if(next_count->get_count() < rep->min)
   {
      // we must take the repeat:
      if(take_first)
      {
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         return true;
      }
      return false;
   }

   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   if(greedy)
   {
      // try and take the repeat if we can:
      if((next_count->get_count() < rep->max) && take_first)
      {
         if(take_second)
         {
            // store position in case we fail:
            push_alt(rep->alt.p);
         }
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         return true;
      }
      else if(take_second)
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
         if((next_count->get_count() < rep->max) && take_first)
         {
            // store position in case we fail:
            push_non_greedy_repeat(rep->next.p);
         }
         pstate = rep->alt.p;
         return true;
      }
      if((next_count->get_count() < rep->max) && take_first)
      {
         // increase the counter:
         ++(*next_count);
         pstate = rep->next.p;
         return true;
      }
   }
   return false;
#ifdef BOOST_BORLANDC
#pragma option pop
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_slow()
{
   std::size_t count = 0;
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   re_syntax_base* psingle = rep->next.p;
   // match compulsory repeats first:
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
      // repeat for as long as we can:
      while(count < rep->max)
      {
         pstate = psingle;
         if(!match_wild())
            break;
         ++count;
      }
      // remember where we got to if this is a leading repeat:
      if((rep->leading) && (count < rep->max))
         restart = position;
      // push backtrack info if available:
      if(count - rep->min)
         push_single_repeat(count, rep, position, saved_state_greedy_single_repeat);
      // jump to next state:
      pstate = rep->alt.p;
      return true;
   }
   else
   {
      // non-greedy, push state and return true if we can skip:
      if(count < rep->max)
         push_single_repeat(count, rep, position, saved_state_rep_slow_dot);
      pstate = rep->alt.p;
      return (position == last) ? (rep->can_be_null & mask_skip) : can_start(*position, rep->_map, mask_skip);
   }
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_dot_repeat_fast()
{
   if(m_match_flags & match_not_dot_null)
      return match_dot_repeat_slow();
   if((static_cast<const re_dot*>(pstate->next.p)->mask & match_any_mask) == 0)
      return match_dot_repeat_slow();

   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   bool greedy = (rep->greedy) && (!(m_match_flags & regex_constants::match_any) || m_independent);   
   std::size_t count = static_cast<std::size_t>((std::min)(static_cast<std::size_t>(::boost::BOOST_REGEX_DETAIL_NS::distance(position, last)), greedy ? rep->max : rep->min));
   if(rep->min > count)
   {
      position = last;
      return false;  // not enough text left to match
   }
   std::advance(position, count);

   if(greedy)
   {
      if((rep->leading) && (count < rep->max))
         restart = position;
      // push backtrack info if available:
      if(count - rep->min)
         push_single_repeat(count, rep, position, saved_state_greedy_single_repeat);
      // jump to next state:
      pstate = rep->alt.p;
      return true;
   }
   else
   {
      // non-greedy, push state and return true if we can skip:
      if(count < rep->max)
         push_single_repeat(count, rep, position, saved_state_rep_fast_dot);
      pstate = rep->alt.p;
      return (position == last) ? (rep->can_be_null & mask_skip) : can_start(*position, rep->_map, mask_skip);
   }
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_char_repeat()
{
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif
#ifdef BOOST_BORLANDC
#pragma option push -w-8008 -w-8066 -w-8004
#endif
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   BOOST_REGEX_ASSERT(1 == static_cast<const re_literal*>(rep->next.p)->length);
   const char_type what = *reinterpret_cast<const char_type*>(static_cast<const re_literal*>(rep->next.p) + 1);
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
      while((position != end) && (traits_inst.translate(*position, icase) == what))
      {
         ++position;
      }
      count = (unsigned)::boost::BOOST_REGEX_DETAIL_NS::distance(origin, position);
   }
   else
   {
      while((count < desired) && (position != last) && (traits_inst.translate(*position, icase) == what))
      {
         ++position;
         ++count;
      }
   }

   if(count < rep->min)
      return false;

   if(greedy)
   {
      if((rep->leading) && (count < rep->max))
         restart = position;
      // push backtrack info if available:
      if(count - rep->min)
         push_single_repeat(count, rep, position, saved_state_greedy_single_repeat);
      // jump to next state:
      pstate = rep->alt.p;
      return true;
   }
   else
   {
      // non-greedy, push state and return true if we can skip:
      if(count < rep->max)
         push_single_repeat(count, rep, position, saved_state_rep_char);
      pstate = rep->alt.p;
      return (position == last) ? (rep->can_be_null & mask_skip) : can_start(*position, rep->_map, mask_skip);
   }
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

   if(count < rep->min)
      return false;

   if(greedy)
   {
      if((rep->leading) && (count < rep->max))
         restart = position;
      // push backtrack info if available:
      if(count - rep->min)
         push_single_repeat(count, rep, position, saved_state_greedy_single_repeat);
      // jump to next state:
      pstate = rep->alt.p;
      return true;
   }
   else
   {
      // non-greedy, push state and return true if we can skip:
      if(count < rep->max)
         push_single_repeat(count, rep, position, saved_state_rep_short_set);
      pstate = rep->alt.p;
      return (position == last) ? (rep->can_be_null & mask_skip) : can_start(*position, rep->_map, mask_skip);
   }
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
   typedef typename traits::char_class_type m_type;
   const re_repeat* rep = static_cast<const re_repeat*>(pstate);
   const re_set_long<m_type>* set = static_cast<const re_set_long<m_type>*>(pstate->next.p);
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

   if(count < rep->min)
      return false;

   if(greedy)
   {
      if((rep->leading) && (count < rep->max))
         restart = position;
      // push backtrack info if available:
      if(count - rep->min)
         push_single_repeat(count, rep, position, saved_state_greedy_single_repeat);
      // jump to next state:
      pstate = rep->alt.p;
      return true;
   }
   else
   {
      // non-greedy, push state and return true if we can skip:
      if(count < rep->max)
         push_single_repeat(count, rep, position, saved_state_rep_long_set);
      pstate = rep->alt.p;
      return (position == last) ? (rep->can_be_null & mask_skip) : can_start(*position, rep->_map, mask_skip);
   }
#ifdef BOOST_BORLANDC
#pragma option pop
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_recursion()
{
   BOOST_REGEX_ASSERT(pstate->type == syntax_element_recurse);
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
   // Backup call stack:
   //
   push_recursion_pop();
   //
   // Set new call stack:
   //
   if(recursion_stack.capacity() == 0)
   {
      recursion_stack.reserve(50);
   }
   recursion_stack.push_back(recursion_info<results_type>());
   recursion_stack.back().preturn_address = pstate->next.p;
   recursion_stack.back().results = *m_presult;
   pstate = static_cast<const re_jump*>(pstate)->alt.p;
   recursion_stack.back().idx = static_cast<const re_brace*>(pstate)->index;
   recursion_stack.back().location_of_start = position;
   //if(static_cast<const re_recurse*>(pstate)->state_id > 0)
   {
      push_repeater_count(-(2 + static_cast<const re_brace*>(pstate)->index), &next_count);
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
            pstate = recursion_stack.back().preturn_address;
            *m_presult = recursion_stack.back().results;
            push_recursion(recursion_stack.back().idx, recursion_stack.back().preturn_address, m_presult, &recursion_stack.back().results);
            recursion_stack.pop_back();
            push_repeater_count(-(2 + index), &next_count);
         }
      }
   }
   else if((index < 0) && (index != -4))
   {
      // matched forward lookahead:
      pstate = 0;
      return true;
   }
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_match()
{
   if(!recursion_stack.empty())
   {
      BOOST_REGEX_ASSERT(0 == recursion_stack.back().idx);
      pstate = recursion_stack.back().preturn_address;
      push_recursion(recursion_stack.back().idx, recursion_stack.back().preturn_address, m_presult, &recursion_stack.back().results);
      *m_presult = recursion_stack.back().results;
      recursion_stack.pop_back();
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
   // Ideally we would just junk all the states that are on the stack,
   // however we might not unwind correctly in that case, so for now,
   // just mark that we don't backtrack into whatever is left (or rather
   // we'll unwind it unconditionally without pausing to try other matches).

   switch(static_cast<const re_commit*>(pstate)->action)
   {
   case commit_commit:
      restart = last;
      break;
   case commit_skip:
      if(base != position)
      {
         restart = position;
         // Have to decrement restart since it will get incremented again later:
         --restart;
      }
      break;
   case commit_prune:
      break;
   }

   saved_state* pmp = m_backup_state;
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = m_backup_state;
      --pmp;
   }
   (void) new (pmp)saved_state(16);
   m_backup_state = pmp;
   pstate = pstate->next.p;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::match_then()
{
   // Just leave a mark that we need to skip to next alternative:
   saved_state* pmp = m_backup_state;
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = m_backup_state;
      --pmp;
   }
   (void) new (pmp)saved_state(17);
   m_backup_state = pmp;
   pstate = pstate->next.p;
   return true;
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
            const re_syntax_base* sp = pstate;
            match_endmark();
            if(!pstate)
            {
               unwind(true);
               // unwind may leave pstate NULL if we've unwound a forward lookahead, in which
               // case just move to the next state and keep looking...
               if (!pstate)
                  pstate = sp->next.p;
            }
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

/****************************************************************************

Unwind and associated procedures follow, these perform what normal stack
unwinding does in the recursive implementation.

****************************************************************************/

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind(bool have_match)
{
   static unwind_proc_type const s_unwind_table[19] = 
   {
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_end,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_paren,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion_stopper,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_assertion,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_alt,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_repeater_counter,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_extra_block,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_greedy_single_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_slow_dot_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_fast_dot_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_char_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_short_set_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_long_set_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_non_greedy_repeat,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion_pop,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_commit,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_then,
      &perl_matcher<BidiIterator, Allocator, traits>::unwind_case,
   };

   m_recursive_result = have_match;
   m_unwound_lookahead = false;
   m_unwound_alt = false;
   unwind_proc_type unwinder;
   bool cont;
   //
   // keep unwinding our stack until we have something to do:
   //
   do
   {
      unwinder = s_unwind_table[m_backup_state->state_id];
      cont = (this->*unwinder)(m_recursive_result);
   }while(cont);
   //
   // return true if we have more states to try:
   //
   return pstate ? true : false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_end(bool)
{
   pstate = 0;   // nothing left to search
   return false; // end of stack nothing more to search
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_case(bool)
{
   saved_change_case* pmp = static_cast<saved_change_case*>(m_backup_state);
   icase = pmp->icase;
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_paren(bool have_match)
{
   saved_matched_paren<BidiIterator>* pmp = static_cast<saved_matched_paren<BidiIterator>*>(m_backup_state);
   // restore previous values if no match was found:
   if(!have_match)
   {
      m_presult->set_first(pmp->sub.first, pmp->index, pmp->index == 0);
      m_presult->set_second(pmp->sub.second, pmp->index, pmp->sub.matched, pmp->index == 0);
   }
#ifdef BOOST_REGEX_MATCH_EXTRA
   //
   // we have a match, push the capture information onto the stack:
   //
   else if(pmp->sub.matched && (match_extra & m_match_flags))
      ((*m_presult)[pmp->index]).get_captures().push_back(pmp->sub);
#endif
   // unwind stack:
   m_backup_state = pmp+1;
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp);
   return true; // keep looking
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion_stopper(bool)
{
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(m_backup_state++);
   pstate = 0;   // nothing left to search
   return false; // end of stack nothing more to search
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_assertion(bool r)
{
   saved_assertion<BidiIterator>* pmp = static_cast<saved_assertion<BidiIterator>*>(m_backup_state);
   pstate = pmp->pstate;
   position = pmp->position;
   bool result = (r == pmp->positive);
   m_recursive_result = pmp->positive ? r : !r;
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   m_unwound_lookahead = true;
   return !result; // return false if the assertion was matched to stop search.
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_alt(bool r)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   if(!r)
   {
      pstate = pmp->pstate;
      position = pmp->position;
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   m_unwound_alt = !r;
   return r; 
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_repeater_counter(bool)
{
   saved_repeater<BidiIterator>* pmp = static_cast<saved_repeater<BidiIterator>*>(m_backup_state);
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true; // keep looking
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_extra_block(bool)
{
   saved_extra_block* pmp = static_cast<saved_extra_block*>(m_backup_state);
   void* condemmed = m_stack_base;
   m_stack_base = pmp->base;
   m_backup_state = pmp->end;
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp);
   put_mem_block(condemmed);
   return true; // keep looking
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::destroy_single_repeat()
{
   saved_single_repeat<BidiIterator>* p = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(p++);
   m_backup_state = p;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_greedy_single_repeat(bool r)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r) 
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;
   BOOST_REGEX_ASSERT(rep->next.p != 0);
   BOOST_REGEX_ASSERT(rep->alt.p != 0);

   count -= rep->min;
   
   if((m_match_flags & match_partial) && (position == last))
      m_has_partial_match = true;

   BOOST_REGEX_ASSERT(count);
   position = pmp->last_position;

   // backtrack till we can skip out:
   do
   {
      --position;
      --count;
      ++state_count;
   }while(count && !can_start(*position, rep->_map, mask_skip));

   // if we've hit base, destroy this state:
   if(count == 0)
   {
         destroy_single_repeat();
         if(!can_start(*position, rep->_map, mask_skip))
            return true;
   }
   else
   {
      pmp->count = count + rep->min;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_slow_dot_repeat(bool r)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r) 
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;
   BOOST_REGEX_ASSERT(rep->type == syntax_element_dot_rep);
   BOOST_REGEX_ASSERT(rep->next.p != 0);
   BOOST_REGEX_ASSERT(rep->alt.p != 0);
   BOOST_REGEX_ASSERT(rep->next.p->type == syntax_element_wild);

   BOOST_REGEX_ASSERT(count < rep->max);
   pstate = rep->next.p;
   position = pmp->last_position;

   if(position != last)
   {
      // wind forward until we can skip out of the repeat:
      do
      {
         if(!match_wild())
         {
            // failed repeat match, discard this state and look for another:
            destroy_single_repeat();
            return true;
         }
         ++count;
         ++state_count;
         pstate = rep->next.p;
      }while((count < rep->max) && (position != last) && !can_start(*position, rep->_map, mask_skip));
   }   
   if(position == last)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if((m_match_flags & match_partial) && (position == last) && (position != search_base))
         m_has_partial_match = true;
      if(0 == (rep->can_be_null & mask_skip))
         return true;
   }
   else if(count == rep->max)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if(!can_start(*position, rep->_map, mask_skip))
         return true;
   }
   else
   {
      pmp->count = count;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_fast_dot_repeat(bool r)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r) 
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;

   BOOST_REGEX_ASSERT(count < rep->max);
   position = pmp->last_position;
   if(position != last)
   {

      // wind forward until we can skip out of the repeat:
      do
      {
         ++position;
         ++count;
         ++state_count;
      }while((count < rep->max) && (position != last) && !can_start(*position, rep->_map, mask_skip));
   }

   // remember where we got to if this is a leading repeat:
   if((rep->leading) && (count < rep->max))
      restart = position;
   if(position == last)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if((m_match_flags & match_partial) && (position == last) && (position != search_base))
         m_has_partial_match = true;
      if(0 == (rep->can_be_null & mask_skip))
         return true;
   }
   else if(count == rep->max)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if(!can_start(*position, rep->_map, mask_skip))
         return true;
   }
   else
   {
      pmp->count = count;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_char_repeat(bool r)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r) 
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;
   pstate = rep->next.p;
   const char_type what = *reinterpret_cast<const char_type*>(static_cast<const re_literal*>(pstate) + 1);
   position = pmp->last_position;

   BOOST_REGEX_ASSERT(rep->type == syntax_element_char_rep);
   BOOST_REGEX_ASSERT(rep->next.p != 0);
   BOOST_REGEX_ASSERT(rep->alt.p != 0);
   BOOST_REGEX_ASSERT(rep->next.p->type == syntax_element_literal);
   BOOST_REGEX_ASSERT(count < rep->max);

   if(position != last)
   {
      // wind forward until we can skip out of the repeat:
      do
      {
         if(traits_inst.translate(*position, icase) != what)
         {
            // failed repeat match, discard this state and look for another:
            destroy_single_repeat();
            return true;
         }
         ++count;
         ++ position;
         ++state_count;
         pstate = rep->next.p;
      }while((count < rep->max) && (position != last) && !can_start(*position, rep->_map, mask_skip));
   }   
   // remember where we got to if this is a leading repeat:
   if((rep->leading) && (count < rep->max))
      restart = position;
   if(position == last)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if((m_match_flags & match_partial) && (position == last) && (position != search_base))
         m_has_partial_match = true;
      if(0 == (rep->can_be_null & mask_skip))
         return true;
   }
   else if(count == rep->max)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if(!can_start(*position, rep->_map, mask_skip))
         return true;
   }
   else
   {
      pmp->count = count;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_short_set_repeat(bool r)
{
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r) 
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;
   pstate = rep->next.p;
   const unsigned char* map = static_cast<const re_set*>(rep->next.p)->_map;
   position = pmp->last_position;

   BOOST_REGEX_ASSERT(rep->type == syntax_element_short_set_rep);
   BOOST_REGEX_ASSERT(rep->next.p != 0);
   BOOST_REGEX_ASSERT(rep->alt.p != 0);
   BOOST_REGEX_ASSERT(rep->next.p->type == syntax_element_set);
   BOOST_REGEX_ASSERT(count < rep->max);
   
   if(position != last)
   {
      // wind forward until we can skip out of the repeat:
      do
      {
         if(!map[static_cast<unsigned char>(traits_inst.translate(*position, icase))])
         {
            // failed repeat match, discard this state and look for another:
            destroy_single_repeat();
            return true;
         }
         ++count;
         ++ position;
         ++state_count;
         pstate = rep->next.p;
      }while((count < rep->max) && (position != last) && !can_start(*position, rep->_map, mask_skip));
   }   
   // remember where we got to if this is a leading repeat:
   if((rep->leading) && (count < rep->max))
      restart = position;
   if(position == last)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if((m_match_flags & match_partial) && (position == last) && (position != search_base))
         m_has_partial_match = true;
      if(0 == (rep->can_be_null & mask_skip))
         return true;
   }
   else if(count == rep->max)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if(!can_start(*position, rep->_map, mask_skip))
         return true;
   }
   else
   {
      pmp->count = count;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_long_set_repeat(bool r)
{
   typedef typename traits::char_class_type m_type;
   saved_single_repeat<BidiIterator>* pmp = static_cast<saved_single_repeat<BidiIterator>*>(m_backup_state);

   // if we have a match, just discard this state:
   if(r)
   {
      destroy_single_repeat();
      return true;
   }

   const re_repeat* rep = pmp->rep;
   std::size_t count = pmp->count;
   pstate = rep->next.p;
   const re_set_long<m_type>* set = static_cast<const re_set_long<m_type>*>(pstate);
   position = pmp->last_position;

   BOOST_REGEX_ASSERT(rep->type == syntax_element_long_set_rep);
   BOOST_REGEX_ASSERT(rep->next.p != 0);
   BOOST_REGEX_ASSERT(rep->alt.p != 0);
   BOOST_REGEX_ASSERT(rep->next.p->type == syntax_element_long_set);
   BOOST_REGEX_ASSERT(count < rep->max);

   if(position != last)
   {
      // wind forward until we can skip out of the repeat:
      do
      {
         if(position == re_is_set_member(position, last, set, re.get_data(), icase))
         {
            // failed repeat match, discard this state and look for another:
            destroy_single_repeat();
            return true;
         }
         ++position;
         ++count;
         ++state_count;
         pstate = rep->next.p;
      }while((count < rep->max) && (position != last) && !can_start(*position, rep->_map, mask_skip));
   }   
   // remember where we got to if this is a leading repeat:
   if((rep->leading) && (count < rep->max))
      restart = position;
   if(position == last)
   {
      // can't repeat any more, remove the pushed state:
      destroy_single_repeat();
      if((m_match_flags & match_partial) && (position == last) && (position != search_base))
         m_has_partial_match = true;
      if(0 == (rep->can_be_null & mask_skip))
         return true;
   }
   else if(count == rep->max)
   {
      // can't repeat any more, remove the pushed state: 
      destroy_single_repeat();
      if(!can_start(*position, rep->_map, mask_skip))
         return true;
   }
   else
   {
      pmp->count = count;
      pmp->last_position = position;
   }
   pstate = rep->alt.p;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_non_greedy_repeat(bool r)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   if(!r)
   {
      position = pmp->position;
      pstate = pmp->pstate;
      ++(*next_count);
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return r;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion(bool r)
{
   // We are backtracking back inside a recursion, need to push the info
   // back onto the recursion stack, and do so unconditionally, otherwise
   // we can get mismatched pushes and pops...
   saved_recursion<results_type>* pmp = static_cast<saved_recursion<results_type>*>(m_backup_state);
   if (!r)
   {
      recursion_stack.push_back(recursion_info<results_type>());
      recursion_stack.back().idx = pmp->recursion_id;
      recursion_stack.back().preturn_address = pmp->preturn_address;
      recursion_stack.back().results = pmp->prior_results;
      recursion_stack.back().location_of_start = position;
      *m_presult = pmp->internal_results;
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_recursion_pop(bool r)
{
   // Backtracking out of a recursion, we must pop state off the recursion
   // stack unconditionally to ensure matched pushes and pops:
   saved_state* pmp = static_cast<saved_state*>(m_backup_state);
   if (!r && !recursion_stack.empty())
   {
      *m_presult = recursion_stack.back().results;
      position = recursion_stack.back().location_of_start;
      recursion_stack.pop_back();
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
void perl_matcher<BidiIterator, Allocator, traits>::push_recursion_pop()
{
   saved_state* pmp = static_cast<saved_state*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_state*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_state(15);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_commit(bool b)
{
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(m_backup_state++);
   while(unwind(b) && !m_unwound_lookahead){}
   if(m_unwound_lookahead && pstate)
   {
      //
      // If we stop because we just unwound an assertion, put the
      // commit state back on the stack again:
      //
      m_unwound_lookahead = false;
      saved_state* pmp = m_backup_state;
      --pmp;
      if(pmp < m_stack_base)
      {
         extend_stack();
         pmp = m_backup_state;
         --pmp;
      }
      (void) new (pmp)saved_state(16);
      m_backup_state = pmp;
   }
   // This prevents us from stopping when we exit from an independent sub-expression:
   m_independent = false;
   return false;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_then(bool b)
{
   // Unwind everything till we hit an alternative:
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(m_backup_state++);
   bool result = false;
   while((result = unwind(b)) && !m_unwound_alt){}
   // We're now pointing at the next alternative, need one more backtrack 
   // since *all* the other alternatives must fail once we've reached a THEN clause:
   if(result && m_unwound_alt)
      unwind(b);
   return false;
}

/*
template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_parenthesis_pop(bool r)
{
   saved_state* pmp = static_cast<saved_state*>(m_backup_state);
   if(!r)
   {
      --parenthesis_stack_position;
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
void perl_matcher<BidiIterator, Allocator, traits>::push_parenthesis_pop()
{
   saved_state* pmp = static_cast<saved_state*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_state*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_state(16);
   m_backup_state = pmp;
}

template <class BidiIterator, class Allocator, class traits>
bool perl_matcher<BidiIterator, Allocator, traits>::unwind_parenthesis_push(bool r)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   if(!r)
   {
      parenthesis_stack[parenthesis_stack_position++] = pmp->position;
   }
   boost::BOOST_REGEX_DETAIL_NS::inplace_destroy(pmp++);
   m_backup_state = pmp;
   return true;
}

template <class BidiIterator, class Allocator, class traits>
inline void perl_matcher<BidiIterator, Allocator, traits>::push_parenthesis_push(BidiIterator p)
{
   saved_position<BidiIterator>* pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
   --pmp;
   if(pmp < m_stack_base)
   {
      extend_stack();
      pmp = static_cast<saved_position<BidiIterator>*>(m_backup_state);
      --pmp;
   }
   (void) new (pmp)saved_position<BidiIterator>(0, p, 17);
   m_backup_state = pmp;
}
*/
} // namespace BOOST_REGEX_DETAIL_NS
} // namespace boost

#ifdef BOOST_MSVC
#  pragma warning(pop)
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
