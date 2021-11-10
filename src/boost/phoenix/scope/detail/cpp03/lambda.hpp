
#if !defined(BOOST_PHOENIX_DONT_USE_PREPROCESSED_FILES)

#include <boost/phoenix/support/iterate.hpp>
#include <boost/phoenix/scope/detail/cpp03/preprocessed/lambda.hpp>

#else

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/lambda_" BOOST_PHOENIX_LIMIT_STR ".hpp")
#endif

/*==============================================================================
    Copyright (c) 2001-2010 Joel de Guzman
    Copyright (c) 2004 Daniel Wallin
    Copyright (c) 2010 Thomas Heller
    Copyright (c) 2016 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_NAME lambda_actor_gen
#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_FUNCTION operator()
#define BOOST_PHOENIX_SCOPE_ACTOR_GEN_CONST const
    #include <boost/phoenix/scope/detail/cpp03/local_gen.hpp>
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_NAME
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_FUNCTION
#undef BOOST_PHOENIX_SCOPE_ACTOR_GEN_CONST

#if defined(__WAVE__) && defined(BOOST_PHOENIX_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif
