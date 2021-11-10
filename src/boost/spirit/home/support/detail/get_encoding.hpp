//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_GET_ENCODING_JANUARY_13_2009_1255PM)
#define BOOST_SPIRIT_GET_ENCODING_JANUARY_13_2009_1255PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { namespace detail
{
    template <typename Modifiers, typename Encoding>
    struct get_implicit_encoding
    {
        // Extract the implicit encoding from the Modifiers
        // If one is not found, Encoding is used. The explicit
        // encoding is the first viable encoding that can be
        // extracted from the Modifiers (there can be more than one).

        typedef typename
            mpl::find_if<
                char_encodings,
                has_modifier<Modifiers, tag::char_encoding_base<mpl::_1> >
            >::type
        iter;

        typedef typename
            mpl::eval_if<
                is_same<iter, typename mpl::end<char_encodings>::type>,
                mpl::identity<Encoding>,
                mpl::deref<iter>
            >::type
        type;
    };

    template <typename Modifiers, typename Encoding>
    struct get_encoding
    {
        // Extract the explicit encoding from the Modifiers
        // If one is not found, get implicit encoding (see above).
        // Explicit encoding is the encoding explicitly declared
        // using the encoding[c] directive.

        typedef typename
            mpl::find_if<
                char_encodings,
                has_modifier<Modifiers, tag::char_code<tag::encoding, mpl::_1> >
            >::type
        iter;

        typedef typename
            mpl::eval_if<
                is_same<iter, typename mpl::end<char_encodings>::type>,
                get_implicit_encoding<Modifiers, Encoding>,
                mpl::deref<iter>
            >::type
        type;
    };

    template <typename Modifiers, typename Encoding, bool case_modifier = false>
    struct get_encoding_with_case : mpl::identity<Encoding> {};

    template <typename Modifiers, typename Encoding>
    struct get_encoding_with_case<Modifiers, Encoding, true>
        : get_encoding<Modifiers, Encoding> {};
}}}

#endif
