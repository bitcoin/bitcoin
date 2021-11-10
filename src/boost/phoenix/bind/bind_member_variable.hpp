/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman
    Copyright (c) 2014      John Fletcher

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef PHOENIX_BIND_BIND_MEMBER_VARIABLE_HPP
#define PHOENIX_BIND_BIND_MEMBER_VARIABLE_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/phoenix/core/expression.hpp>
#include <boost/phoenix/core/detail/function_eval.hpp>
#include <boost/phoenix/bind/detail/member_variable.hpp>

namespace boost { namespace phoenix
{
    template <typename RT, typename ClassT, typename ClassA>
    inline
    typename boost::lazy_disable_if<
      boost::is_member_function_pointer<RT (ClassT::*)>,
        typename detail::expression::function_eval<
            detail::member_variable<RT, RT ClassT::*>
          , ClassA >//::type
      >::type const
    bind(RT ClassT::*mp, ClassA const& obj)
    {
        typedef detail::member_variable<RT, RT ClassT::*> mp_type;
        return
            detail::expression::function_eval<mp_type, ClassA>
                ::make(mp_type(mp), obj);
    }

    template <typename RT, typename ClassT>
    inline
    typename boost::lazy_disable_if<
      boost::is_member_function_pointer<RT (ClassT::*)>,
        typename detail::expression::function_eval<
            detail::member_variable<RT, RT ClassT::*>
          , ClassT >//::type
        >::type const
    bind(RT ClassT::*mp, ClassT& obj)
    {
        typedef detail::member_variable<RT, RT ClassT::*> mp_type;
        return
            detail::expression::function_eval<
                mp_type
              , ClassT
            >::make(mp_type(mp), obj);
    }

}}

#endif
