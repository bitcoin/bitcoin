// Boost.Signals2 library

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP
#define BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP

#include <boost/assert.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/signals2/slot_base.hpp>
#include <boost/signals2/detail/auto_buffer.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/weak_ptr.hpp>

namespace boost {
  namespace signals2 {
    namespace detail {
      template<typename ResultType, typename Function>
        class slot_call_iterator_cache
      {
      public:
        slot_call_iterator_cache(const Function &f_arg):
          f(f_arg),
          connected_slot_count(0),
          disconnected_slot_count(0),
          m_active_slot(0)
        {}

        ~slot_call_iterator_cache()
        {
          if(m_active_slot)
          {
            garbage_collecting_lock<connection_body_base> lock(*m_active_slot);
            m_active_slot->dec_slot_refcount(lock);
          }
        }

        template<typename M>
        void set_active_slot(garbage_collecting_lock<M> &lock,
          connection_body_base *active_slot)
        {
          if(m_active_slot)
            m_active_slot->dec_slot_refcount(lock);
          m_active_slot = active_slot;
          if(m_active_slot)
            m_active_slot->inc_slot_refcount(lock);
        }

        optional<ResultType> result;
        typedef auto_buffer<void_shared_ptr_variant, store_n_objects<10> > tracked_ptrs_type;
        tracked_ptrs_type tracked_ptrs;
        Function f;
        unsigned connected_slot_count;
        unsigned disconnected_slot_count;
        connection_body_base *m_active_slot;
      };

      // Generates a slot call iterator. Essentially, this is an iterator that:
      //   - skips over disconnected slots in the underlying list
      //   - calls the connected slots when dereferenced
      //   - caches the result of calling the slots
      template<typename Function, typename Iterator, typename ConnectionBody>
      class slot_call_iterator_t
        : public boost::iterator_facade<slot_call_iterator_t<Function, Iterator, ConnectionBody>,
        typename Function::result_type,
        boost::single_pass_traversal_tag,
        typename boost::add_reference<typename boost::add_const<typename Function::result_type>::type>::type >
      {
        typedef boost::iterator_facade<slot_call_iterator_t<Function, Iterator, ConnectionBody>,
          typename Function::result_type,
          boost::single_pass_traversal_tag,
          typename boost::add_reference<typename boost::add_const<typename Function::result_type>::type>::type >
        inherited;

        typedef typename Function::result_type result_type;

        typedef slot_call_iterator_cache<result_type, Function> cache_type;

        friend class boost::iterator_core_access;

      public:
        slot_call_iterator_t(Iterator iter_in, Iterator end_in,
          cache_type &c):
          iter(iter_in), end(end_in),
          cache(&c), callable_iter(end_in)
        {
          lock_next_callable();
        }

        typename inherited::reference
        dereference() const
        {
          if (!cache->result) {
            BOOST_TRY
            {
              cache->result.reset(cache->f(*iter));
            }
            BOOST_CATCH(expired_slot &)
            {
              (*iter)->disconnect();
              BOOST_RETHROW
            }
            BOOST_CATCH_END
          }
          return cache->result.get();
        }

        void increment()
        {
          ++iter;
          lock_next_callable();
          cache->result.reset();
        }

        bool equal(const slot_call_iterator_t& other) const
        {
          return iter == other.iter;
        }

      private:
        typedef garbage_collecting_lock<connection_body_base> lock_type;

        void set_callable_iter(lock_type &lock, Iterator newValue) const
        {
          callable_iter = newValue;
          if(callable_iter == end)
            cache->set_active_slot(lock, 0);
          else
            cache->set_active_slot(lock, (*callable_iter).get());
        }

        void lock_next_callable() const
        {
          if(iter == callable_iter)
          {
            return;
          }
  
          for(;iter != end; ++iter)
          {
            cache->tracked_ptrs.clear();
            lock_type lock(**iter);
            (*iter)->nolock_grab_tracked_objects(lock, std::back_inserter(cache->tracked_ptrs));
            if((*iter)->nolock_nograb_connected())
            {
              ++cache->connected_slot_count;
            }else
            {
              ++cache->disconnected_slot_count;
            }
            if((*iter)->nolock_nograb_blocked() == false)
            {
              set_callable_iter(lock, iter);
              break;
            }
          }
          
          if(iter == end)
          {
            if(callable_iter != end)
            {
              lock_type lock(**callable_iter);
              set_callable_iter(lock, end);
            }
          }
        }

        mutable Iterator iter;
        Iterator end;
        cache_type *cache;
        mutable Iterator callable_iter;
      };
    } // end namespace detail
  } // end namespace BOOST_SIGNALS_NAMESPACE
} // end namespace boost

#endif // BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP
