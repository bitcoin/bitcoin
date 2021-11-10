// Copyright Frank Mori Hess 2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

// This file is included iteratively, and should not be protected from multiple inclusion

#define BOOST_SIGNALS2_NUM_ARGS BOOST_PP_ITERATION()

namespace boost
{
  namespace signals2
  {
    namespace detail
    {
      template<unsigned n BOOST_PP_COMMA_IF(BOOST_SIGNALS2_NUM_ARGS)
        BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)>
        class BOOST_SIGNALS2_PREPROCESSED_ARG_N_TYPE_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS);

// template<typename T1, typename T2, ... , typename TN> class preprocessed_arg_typeN<n, T1, T2, ..., TN>{...} ...
#define BOOST_SIGNALS2_PREPROCESSED_ARG_TYPE_CLASS_TEMPLATE_SPECIALIZATION(z, n, data) \
  template<BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(BOOST_SIGNALS2_NUM_ARGS)> \
  class BOOST_SIGNALS2_PREPROCESSED_ARG_N_TYPE_CLASS_NAME(BOOST_SIGNALS2_NUM_ARGS)<n, \
    BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(BOOST_SIGNALS2_NUM_ARGS)> \
  { \
  public: \
    typedef BOOST_PP_CAT(T, BOOST_PP_INC(n)) type; \
  };
      BOOST_PP_REPEAT(BOOST_SIGNALS2_NUM_ARGS, BOOST_SIGNALS2_PREPROCESSED_ARG_TYPE_CLASS_TEMPLATE_SPECIALIZATION, ~)

    } // namespace detail
  } // namespace signals2
} // namespace boost

#undef BOOST_SIGNALS2_NUM_ARGS
