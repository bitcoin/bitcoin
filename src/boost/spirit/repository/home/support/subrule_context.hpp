/*=============================================================================
    Copyright (c) 2009 Francois Barel
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_REPOSITORY_SUPPORT_SUBRULE_CONTEXT_AUGUST_12_2009_0539PM)
#define BOOST_SPIRIT_REPOSITORY_SUPPORT_SUBRULE_CONTEXT_AUGUST_12_2009_0539PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/context.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository
{
    ///////////////////////////////////////////////////////////////////////////
    // subrule_context: special context used with subrules, to pass around
    // the current set of subrule definitions (subrule_group)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Group, typename Attributes, typename Locals>
    struct subrule_context
      : context<Attributes, Locals>
    {
        typedef context<Attributes, Locals> base_type;
        typedef Group group_type;

        subrule_context(
            Group const& group
          , typename Attributes::car_type attribute
        ) : base_type(attribute), group(group)
        {
        }

        template <typename Args, typename Context>
        subrule_context(
            Group const& group
          , typename Attributes::car_type attribute
          , Args const& args
          , Context& caller_context
        ) : base_type(attribute, args, caller_context), group(group)
        {
        }

        subrule_context(Group const& group, Attributes const& attributes)
          : base_type(attributes), group(group)
        {
        }

        Group const& group;
    };
}}}

#endif
