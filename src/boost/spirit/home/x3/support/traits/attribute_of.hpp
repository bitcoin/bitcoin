/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Agustin Berge
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ATTRIBUTE_OF_JAN_7_2012_0914AM)
#define BOOST_SPIRIT_X3_ATTRIBUTE_OF_JAN_7_2012_0914AM

#include <boost/spirit/home/x3/support/utility/sfinae.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Get the attribute type of a component. By default, this gets the
    // Component's attribute_type typedef or instantiates a nested attribute
    // metafunction. Components may specialize this if such an attribute_type
    // is not readily available (e.g. expensive to compute at compile time).
    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Context, typename Enable = void>
    struct attribute_of;

    namespace detail
    {
        template <typename Component, typename Context, typename Enable = void>
        struct default_attribute_of;

        template <typename Component, typename Context>
        struct default_attribute_of<Component, Context,
            typename disable_if_substitution_failure<
                typename Component::attribute_type>::type>
          : mpl::identity<typename Component::attribute_type> {};

        template <typename Component, typename Context>
        struct default_attribute_of<Component, Context,
            typename disable_if_substitution_failure<
                typename Component::template attribute<Context>::type>::type>
          : Component::template attribute<Context> {};

        template <typename Component, typename Context>
        struct default_attribute_of<Component, Context,
            typename enable_if_c<Component::is_pass_through_unary>::type>
          : attribute_of<typename Component::subject_type, Context>{};
    }

    template <typename Component, typename Context, typename Enable>
    struct attribute_of : detail::default_attribute_of<Component, Context> {};
}}}}

#endif
