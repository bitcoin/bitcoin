/*=============================================================================
    Copyright (c) 2019 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_PSEUDO_ATTRIBUTE_OF_MAY_15_2019_1012PM)
#define BOOST_SPIRIT_X3_PSEUDO_ATTRIBUTE_OF_MAY_15_2019_1012PM

#include <utility>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // Pseudo attributes are placeholders for parsers that can only know
    // its actual attribute at parse time. This trait customization point
    // provides a mechanism to convert the trait to the actual trait at
    // parse time.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Attribute, typename Iterator
      , typename Enable = void>
    struct pseudo_attribute
    {
        using attribute_type = Attribute;
        using type = Attribute;

        static type&& call(Iterator&, Iterator const&, attribute_type&& attribute)
        {
            return std::forward<type>(attribute);
        }
    };
}}}}

#endif
