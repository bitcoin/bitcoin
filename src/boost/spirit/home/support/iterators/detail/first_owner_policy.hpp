//  Copyright (c) 2001, Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_FIRST_OWNER_POLICY_MAR_16_2007_1108AM)
#define BOOST_SPIRIT_ITERATOR_FIRST_OWNER_POLICY_MAR_16_2007_1108AM

#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>

namespace boost { namespace spirit { namespace iterator_policies
{
    ///////////////////////////////////////////////////////////////////////////
    //  class first_owner
    //  Implementation of an OwnershipPolicy used by multi_pass
    //  This ownership policy dictates that the first iterator created will
    //  determine the lifespan of the shared components.  This works well for
    //  spirit, since no dynamic allocation of iterators is done, and all 
    //  copies are make on the stack.
    //
    //  There is a caveat about using this policy together with the std_deque
    //  StoragePolicy. Since first_owner always returns false from unique(),
    //  std_deque will only release the queued data if clear_queue() is called.
    ///////////////////////////////////////////////////////////////////////////
    struct first_owner
    {
        ///////////////////////////////////////////////////////////////////////
        struct unique : detail::default_ownership_policy
        {
            unique() : first(true) {}
            unique(unique const&) : first(false) {}

            // return true to indicate deletion of resources
            template <typename MultiPass>
            static bool release(MultiPass& mp)
            {
                return mp.first;
            }

            // use swap from default policy
            // if we're the first, we still remain the first, even if assigned
            // to, so don't swap first.  swap is only called from operator=

            template <typename MultiPass>
            static bool is_unique(MultiPass const&) 
            {
                return false; // no way to know, so always return false
            }

        protected:
            bool first;
        };

        ////////////////////////////////////////////////////////////////////////
        struct shared {};   // no shared data
    };

}}}

#endif
