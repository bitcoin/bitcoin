// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_NO_FSM_CHECK_H
#define BOOST_MSM_BACK_NO_FSM_CHECK_H

#include <boost/mpl/assert.hpp>

#include <boost/msm/back/metafunctions.hpp>

namespace boost { namespace msm { namespace back
{
    struct no_fsm_check
    {
        typedef int fsm_check;
        // no fsm structure checking
        template <class Fsm>
        static void check_orthogonality()
        {
        }
        template <class Fsm>
        static void check_unreachable_states()
        {
        }
    };

} } }//boost::msm::back


#endif //BOOST_MSM_BACK_NO_FSM_CHECK_H
