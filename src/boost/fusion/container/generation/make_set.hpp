/*=============================================================================
    Copyright (c) 2014-2015 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_MAKE_SET_11112014_2255
#define FUSION_MAKE_SET_11112014_2255

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/set/set.hpp>

#if !defined(BOOST_FUSION_HAS_VARIADIC_SET)
# include <boost/fusion/container/generation/detail/pp_make_set.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 variadic interface
///////////////////////////////////////////////////////////////////////////////

#include <boost/fusion/support/detail/as_fusion_element.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <utility>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename ...T>
        struct make_set
        {
            typedef set<
                typename detail::as_fusion_element<
                    typename remove_const<
                        typename remove_reference<T>::type
                    >::type
                >::type...
            > type;
        };
    }

    template <typename ...T>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::make_set<T...>::type
    make_set(T&&... arg)
    {
        return typename result_of::make_set<T...>::type(std::forward<T>(arg)...);
    }
 }}


#endif
#endif

