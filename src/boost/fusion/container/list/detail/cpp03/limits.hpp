/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_LIST_LIMITS_07172005_0112)
#define FUSION_LIST_LIMITS_07172005_0112

#include <boost/fusion/support/detail/pp_round.hpp>

#if !defined(FUSION_MAX_LIST_SIZE)
# define FUSION_MAX_LIST_SIZE 10
#else
# if FUSION_MAX_LIST_SIZE < 3
#   undef FUSION_MAX_LIST_SIZE
#   define FUSION_MAX_LIST_SIZE 10
# endif
#endif

#define FUSION_MAX_LIST_SIZE_STR BOOST_PP_STRINGIZE(BOOST_FUSION_PP_ROUND_UP(FUSION_MAX_LIST_SIZE))

#endif
