/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_SUPPORT_SEQUENCE_BASE_ID_HPP
#define BOOST_SPIRIT_SUPPORT_SEQUENCE_BASE_ID_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/has_xxx.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/spirit/home/support/attributes.hpp>

namespace boost { namespace spirit { namespace traits
{
    namespace detail
    {
        BOOST_MPL_HAS_XXX_TRAIT_DEF(sequence_base_id)
    }

    // We specialize this for sequences (see support/attributes.hpp).
    // For sequences, we only wrap the attribute in a tuple IFF
    // it is not already a fusion tuple.
    //
    // Note: in the comment above, "sequence" is a spirit sequence
    // component (parser or generator), and a tuple is a fusion sequence 
    // (to avoid terminology confusion).
    template <typename Derived, typename Attribute>
    struct pass_attribute<Derived, Attribute,
        typename enable_if<detail::has_sequence_base_id<Derived> >::type>
      : wrap_if_not_tuple<Attribute> {};

}}}

#endif
