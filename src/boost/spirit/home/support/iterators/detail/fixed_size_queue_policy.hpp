//  Copyright (c) 2001 Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_FIXED_SIZE_QUEUE_POLICY_MAR_16_2007_1134AM)
#define BOOST_SPIRIT_ITERATOR_FIXED_SIZE_QUEUE_POLICY_MAR_16_2007_1134AM

#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/spirit/home/support/iterators/detail/fixed_size_queue.hpp>
#include <boost/assert.hpp>
#include <cstdlib>

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  class fixed_size_queue
    //  Implementation of the StoragePolicy used by multi_pass
    //  fixed_size_queue keeps a circular buffer (implemented by
    //  boost::spirit::fixed_size_queue class) that is size N+1 and stores N 
    //  elements.
    // 
    //  It is up to the user to ensure that there is enough look ahead for 
    //  their grammar. Currently there is no way to tell if an iterator is 
    //  pointing to forgotten data. The leading iterator will put an item in 
    //  the queue and remove one when it is incremented. No dynamic allocation 
    //  is done, except on creation of the queue (fixed_size_queue constructor).
    ///////////////////////////////////////////////////////////////////////////
    template <std::size_t N>
    struct fixed_size_queue
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename Value>
        class unique : public detail::default_storage_policy
        {
        private:
            typedef detail::fixed_size_queue<Value, N> queue_type;

        protected:
            unique() {}

            unique(unique const& x)
              : queued_position(x.queued_position) {}

            void swap(unique& x)
            {
                boost::swap(queued_position, x.queued_position);
            }

            //  This is called when the iterator is dereferenced. It's a 
            //  template method so we can recover the type of the multi_pass 
            //  iterator and access the m_input data member.
            template <typename MultiPass>
            static typename MultiPass::reference 
            dereference(MultiPass const& mp)
            {
                if (!mp.queued_position.get_position().is_initialized())
                   mp.queued_position.get_position().set_queue(&mp.shared()->queued_elements);

                if (mp.queued_position == mp.shared()->queued_elements.end())
                    return MultiPass::get_input(mp);

                return *mp.queued_position;
            }

            //  This is called when the iterator is incremented. It's a 
            //  template method so we can recover the type of the multi_pass 
            //  iterator and access the m_input data member.
            template <typename MultiPass>
            static void increment(MultiPass& mp)
            {
                if (!mp.queued_position.get_position().is_initialized())
                    mp.queued_position.get_position().set_queue(&mp.shared()->queued_elements);

                if (mp.queued_position == mp.shared()->queued_elements.end())
                {
                    // don't let the queue get larger than N
                    if (mp.shared()->queued_elements.size() >= N)
                        mp.shared()->queued_elements.pop_front();

                    mp.shared()->queued_elements.push_back(
                        MultiPass::get_input(mp));
                    MultiPass::advance_input(mp);
                }
                ++mp.queued_position;
            }

            // clear_queue is a no-op

            // called to determine whether the iterator is an eof iterator
            template <typename MultiPass>
            static bool is_eof(MultiPass const& mp)
            {
                return mp.queued_position == mp.shared()->queued_elements.end() &&
                       MultiPass::input_at_eof(mp);
            }

            // called by operator==
            template <typename MultiPass>
            static bool equal_to(MultiPass const& mp, MultiPass const& x) 
            {
                return mp.queued_position == x.queued_position;
            }

            // called by operator<
            template <typename MultiPass>
            static bool less_than(MultiPass const& mp, MultiPass const& x)
            {
                return mp.queued_position < x.queued_position;
            }

        protected:
            mutable typename queue_type::iterator queued_position;
        }; 

        ///////////////////////////////////////////////////////////////////////
        template <typename Value>
        struct shared
        {
            typedef detail::fixed_size_queue<Value, N> queue_type;
            queue_type queued_elements;
        }; 
    }; 

}}}

#endif
