/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_ADT_DETAIL_EXTENSION_HPP
#define BOOST_FUSION_ADAPTED_ADT_DETAIL_EXTENSION_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/fusion/support/as_const.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>

namespace boost { namespace fusion
{
    namespace detail
    {
        template <typename T, typename Dummy>
        struct get_identity
          : remove_const<typename remove_reference<T>::type>
        {};
    }

    namespace extension
    {
        // Overload as_const() to unwrap adt_attribute_proxy.
        template <typename T, int N, bool Const>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename adt_attribute_proxy<T, N, Const>::type as_const(const adt_attribute_proxy<T, N, Const>& proxy)
        {
            return proxy.get();
        }
    }
}}

#endif
