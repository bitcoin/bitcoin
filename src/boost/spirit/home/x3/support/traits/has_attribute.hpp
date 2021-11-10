/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Agustin Berge
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_HAS_ATTRIBUTE_JUN_6_2012_1714PM)
#define BOOST_SPIRIT_X3_HAS_ATTRIBUTE_JUN_6_2012_1714PM

#include <boost/spirit/home/x3/support/traits/attribute_of.hpp>
#include <boost/spirit/home/x3/support/utility/sfinae.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit { namespace x3
{
   struct unused_type;
}}}

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Whether a component has an attribute. By default, this compares the 
    // component attribute against unused_type. If the component provides a
    // nested constant expression has_attribute as a hint, that value is used
    // instead. Components may specialize this.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Context, typename Enable = void>
    struct has_attribute;
    
    namespace detail
    {
        template <typename Component, typename Context, typename Enable = void>
        struct default_has_attribute
          : mpl::not_<is_same<unused_type,
                typename attribute_of<Component, Context>::type>> {};

        template <typename Component, typename Context>
        struct default_has_attribute<Component, Context,
            typename disable_if_substitution_failure<
                mpl::bool_<Component::has_attribute>>::type>
          : mpl::bool_<Component::has_attribute> {};
    
        template <typename Component, typename Context>
        struct default_has_attribute<Component, Context,
            typename enable_if_c<Component::is_pass_through_unary>::type>
          : has_attribute<typename Component::subject_type, Context> {};
    }
    
    template <typename Component, typename Context, typename Enable>
    struct has_attribute : detail::default_has_attribute<Component, Context> {};

}}}}

#endif
