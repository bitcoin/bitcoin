/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2012 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_ATTRIBUTE_TRANSFORM_JAN_8_2012_0721PM)
#define BOOST_SPIRIT_X3_ATTRIBUTE_TRANSFORM_JAN_8_2012_0721PM

#include <boost/mpl/identity.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  transform_attribute
    //
    //  Sometimes the user needs to transform the attribute types for certain
    //  attributes. This template can be used as a customization point, where
    //  the user is able specify specific transformation rules for any attribute
    //  type.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Exposed, typename Transformed, typename Tag
      , typename Enable = void>
    struct transform_attribute;
}}}}

#endif
