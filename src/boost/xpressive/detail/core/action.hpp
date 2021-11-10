///////////////////////////////////////////////////////////////////////////////
// action.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_ACTION_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_ACTION_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/match_results.hpp> // for type_info_less

namespace boost { namespace xpressive { namespace detail
{

    ///////////////////////////////////////////////////////////////////////////////
    // actionable
    //
    struct actionable
    {
        virtual ~actionable() {}
        virtual void execute(action_args_type *) const {}

        actionable()
          : next(0)
        {}

        actionable const *next;
    };

}}} // namespace boost::xpressive::detail

#endif
