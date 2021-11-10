// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2009.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_PREPROCESSED_ARG_TYPE_HPP
#define BOOST_SIGNALS2_PREPROCESSED_ARG_TYPE_HPP

#include <boost/preprocessor/repetition.hpp>
#include <boost/signals2/detail/signals_common_macros.hpp>

#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_INC(BOOST_SIGNALS2_MAX_ARGS))
#define BOOST_PP_FILENAME_1 <boost/signals2/detail/preprocessed_arg_type_template.hpp>
#include BOOST_PP_ITERATE()

namespace boost
{
  namespace signals2
  {
    namespace detail
    {
      struct std_functional_base
      {};
    } // namespace detail
  } // namespace signals2
} // namespace boost

#endif // BOOST_SIGNALS2_PREPROCESSED_ARG_TYPE_HPP
