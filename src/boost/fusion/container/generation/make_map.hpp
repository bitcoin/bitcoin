/*=============================================================================
    Copyright (c) 2001-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MAKE_MAP_07222005_1247)
#define FUSION_MAKE_MAP_07222005_1247

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/map/map.hpp>

#if !defined(BOOST_FUSION_HAS_VARIADIC_MAP)
# include <boost/fusion/container/generation/detail/pp_make_map.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 variadic interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/support/detail/as_fusion_element.hpp>
#include <boost/fusion/support/pair.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename ...Key>
        struct make_map
        {
            template <typename ...T>
            struct apply
            {
                typedef map<
                    fusion::pair<
                        Key
                      , typename detail::as_fusion_element<T>::type
                    >...>
                type;
            };
        };
    }

    template <typename ...Key, typename ...T>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline map<
        fusion::pair<
            Key
          , typename detail::as_fusion_element<T>::type
        >...>
    make_map(T const&... arg)
    {
        typedef map<
            fusion::pair<
                Key
              , typename detail::as_fusion_element<T>::type
            >...>
        result_type;

        return result_type(arg...);
    }
 }}

#endif
#endif
