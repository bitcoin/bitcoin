/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    Copyright (c) 2013 Agustin Berge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_HANDLES_CONTAINER_DEC_18_2010_0920AM)
#define BOOST_SPIRIT_X3_HANDLES_CONTAINER_DEC_18_2010_0920AM

#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Whether a component handles container attributes intrinsically
    // (or whether container attributes need to be split up separately).
    // By default, this gets the Component's handles_container nested value.
    // Components may specialize this if such a handles_container is not 
    // readily available (e.g. expensive to compute at compile time).
    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Context, typename Enable = void>
    struct handles_container : mpl::bool_<Component::handles_container> {};

}}}}

#endif
