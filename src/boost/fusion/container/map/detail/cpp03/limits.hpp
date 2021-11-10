/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_MAP_LIMITS_07212005_1104)
#define FUSION_MAP_LIMITS_07212005_1104

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/vector/detail/cpp03/limits.hpp>

#if !defined(FUSION_MAX_MAP_SIZE)
# define FUSION_MAX_MAP_SIZE FUSION_MAX_VECTOR_SIZE
#else
# if FUSION_MAX_MAP_SIZE < 3
#   undef FUSION_MAX_MAP_SIZE
#   if (FUSION_MAX_VECTOR_SIZE > 10)
#       define FUSION_MAX_MAP_SIZE 10
#   else
#       define FUSION_MAX_MAP_SIZE FUSION_MAX_VECTOR_SIZE
#   endif
# endif
#endif

#define FUSION_MAX_MAP_SIZE_STR BOOST_PP_STRINGIZE(BOOST_FUSION_PP_ROUND_UP(FUSION_MAX_MAP_SIZE))

#endif
