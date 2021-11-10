/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_ACCESS_04182005_0737)
#define FUSION_ACCESS_04182005_0737

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename T>
    struct ref_result
    {
        typedef typename add_reference<T>::type type;
    };

    template <typename T>
    struct cref_result
    {
        typedef typename
            add_reference<
                typename add_const<T>::type
            >::type
        type;
    };

    template <typename T>
    struct call_param
    {
        typedef T const& type;
    };

    template <typename T>
    struct call_param<T&>
    {
        typedef T& type;
    };

    template <typename T>
    struct call_param<T const>
    {
        typedef T const& type;
    };

    template <typename T>
    struct call_param<T volatile>
    {
        typedef T const& type;
    };

    template <typename T>
    struct call_param<T const volatile>
    {
        typedef T const& type;
    };

}}}

#endif

