/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_VARIANT_FIND_SUBSTITUTE_APR_18_2014_930AM)
#define BOOST_SPIRIT_X3_VARIANT_FIND_SUBSTITUTE_APR_18_2014_930AM

#include <boost/spirit/home/x3/support/traits/is_substitute.hpp>
#include <boost/mpl/find.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename Variant, typename Attribute>
    struct variant_find_substitute
    {
        // Get the type from the variant that can be a substitute for Attribute.
        // If none is found, just return Attribute

        typedef Variant variant_type;
        typedef typename variant_type::types types;
        typedef typename mpl::end<types>::type end;

        typedef typename mpl::find<types, Attribute>::type iter_1;

        typedef typename
            mpl::eval_if<
                is_same<iter_1, end>,
                mpl::find_if<types, traits::is_substitute<mpl::_1, Attribute> >,
                mpl::identity<iter_1>
            >::type
        iter;

        typedef typename
            mpl::eval_if<
                is_same<iter, end>,
                mpl::identity<Attribute>,
                mpl::deref<iter>
            >::type
        type;
    };
    
    template <typename Variant>
    struct variant_find_substitute<Variant, Variant>
        : mpl::identity<Variant> {};

}}}}

#endif
