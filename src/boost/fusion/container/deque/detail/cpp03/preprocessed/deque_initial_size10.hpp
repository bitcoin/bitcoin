/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/
namespace boost { namespace fusion { namespace detail
{
    template<typename T0 , typename T1 , typename T2 , typename T3 , typename T4 , typename T5 , typename T6 , typename T7 , typename T8 , typename T9>
    struct deque_initial_size
    {
        typedef mpl::vector<T0 , T1 , T2 , T3 , T4 , T5 , T6 , T7 , T8 , T9> args;
        typedef typename mpl::find<args, void_>::type first_void;
        typedef typename mpl::distance<typename mpl::begin<args>::type, first_void>::type type;
    };
}}}
