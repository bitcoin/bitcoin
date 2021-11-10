/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_MAP_INDEX_02032013_2233)
#define BOOST_FUSION_MAP_INDEX_02032013_2233

namespace boost { namespace fusion { namespace detail
{
    template <int N>
    struct map_index
    {
        static int const value = N;
    };
}}}

#endif
