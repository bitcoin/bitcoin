// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_COMMON_COMPLETION_EVENT_H
#define BOOST_MSM_FRONT_COMMON_COMPLETION_EVENT_H

namespace boost { namespace msm { namespace front
{

    struct none
    {
        // make every event convertible to none.
        // to support standard-conform implementation of pseudo exits.
        none(){}
        template <class Event>
        none(Event const&){}

        typedef int completion_event;
    };

}}}

#endif //BOOST_MSM_FRONT_COMMON_COMPLETION_EVENT_H

