// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_COMMON_TYPES_H
#define BOOST_MSM_COMMON_TYPES_H

#include <boost/tuple/tuple.hpp>
#include <boost/msm/common.hpp>

namespace boost { namespace msm { namespace back
{
// used for disable_if
template <int> struct dummy { dummy(int) {} };
// return value for transition handling
typedef enum
{
    HANDLED_FALSE=0,
    HANDLED_TRUE =1,
    HANDLED_GUARD_REJECT=2,
    HANDLED_DEFERRED=4
} HandledEnum;

typedef HandledEnum execute_return;

// source of event provided to RTC algorithm
enum EventSourceEnum
{
    EVENT_SOURCE_DEFAULT=0,
    EVENT_SOURCE_DIRECT=1,
    EVENT_SOURCE_DEFERRED=2,
    EVENT_SOURCE_MSG_QUEUE=4
};

typedef unsigned char EventSource;

}}}

#endif //BOOST_MSM_COMMON_TYPES_H

