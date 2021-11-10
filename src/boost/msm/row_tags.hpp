// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_ROW_TAGS_H
#define BOOST_MSM_ROW_TAGS_H



namespace boost { namespace msm
{
//tags
struct a_row_tag {};
struct g_row_tag {};
struct _row_tag {};
struct row_tag {};
// tags for internal transitions
struct a_irow_tag {};
struct g_irow_tag {};
struct _irow_tag {};
struct irow_tag {};
// tags for transitions internal to state machines (not using any substate)
struct sm_a_i_row_tag {};
struct sm_g_i_row_tag {};
struct sm__i_row_tag {};
struct sm_i_row_tag {};

// flags used internally to handle terminate / interrupt states
struct TerminateFlag 
{
    typedef int non_forwarding_flag;
    typedef int event_blocking_flag;
};
struct InterruptedFlag
{
    typedef int non_forwarding_flag;
    typedef int event_blocking_flag;
};
template <class EndEvent>
struct EndInterruptFlag
{
    typedef int non_forwarding_flag;
};

} } // boost::msm
#endif //BOOST_MSM_ROW_TAGS_H

