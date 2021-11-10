/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2010 Eric Niebler
    Copyright (c) 2010-2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

    #define BOOST_PHOENIX_ARGUMENT_N_TYPE(_, N, name)                           \
    typedef                                                                     \
        expression::argument<BOOST_PP_INC(N)>::type                             \
        BOOST_PP_CAT(BOOST_PP_CAT(name, BOOST_PP_INC(N)), _type)                \
        BOOST_ATTRIBUTE_UNUSED;                                                 \
    /**/
    
    #define BOOST_PHOENIX_ARGUMENT_N_INSTANCE(_, N, name)                       \
    expression::argument<BOOST_PP_INC(N)>::type const                           \
        BOOST_ATTRIBUTE_UNUSED                                                  \
        BOOST_PP_CAT(name, BOOST_PP_INC(N)) = {{{}}};                           \
    /**/


    namespace placeholders
    {
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_TYPE, arg)
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_TYPE, _)
#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_INSTANCE, arg)
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_INSTANCE, _)
#endif
    }

    namespace arg_names
    {
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_TYPE, arg)
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_TYPE, _)
#ifndef BOOST_PHOENIX_NO_PREDEFINED_TERMINALS
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_INSTANCE, arg)
        BOOST_PP_REPEAT(BOOST_PHOENIX_ARG_LIMIT, BOOST_PHOENIX_ARGUMENT_N_INSTANCE, _)
#endif
    }

    #undef BOOST_PHOENIX_ARGUMENT_N_TYPE
    #undef BOOST_PHOENIX_ARGUMENT_N_INSTANCE

