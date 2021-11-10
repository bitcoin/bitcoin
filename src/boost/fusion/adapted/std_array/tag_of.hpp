/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_STD_ARRAY_TAG_OF_01062013_1700)
#define BOOST_FUSION_STD_ARRAY_TAG_OF_01062013_1700

#include <boost/fusion/support/tag_of_fwd.hpp>
#include <array>
#include <cstddef>

namespace boost { namespace fusion
{
    struct std_array_tag;
    struct fusion_sequence_tag;

    namespace traits
    {
        template<typename T, std::size_t N>
#if defined(BOOST_NO_PARTIAL_SPECIALIZATION_IMPLICIT_DEFAULT_ARGS)
        struct tag_of<std::array<T,N>, void >
#else
        struct tag_of<std::array<T,N> >
#endif
        {
            typedef std_array_tag type;
        };
    }
}}

namespace boost { namespace mpl
{
    template<typename>
    struct sequence_tag;

    template<typename T, std::size_t N>
    struct sequence_tag<std::array<T,N> >
    {
        typedef fusion::fusion_sequence_tag type;
    };

    template<typename T, std::size_t N>
    struct sequence_tag<std::array<T,N> const>
    {
        typedef fusion::fusion_sequence_tag type;
    };
}}

#endif
