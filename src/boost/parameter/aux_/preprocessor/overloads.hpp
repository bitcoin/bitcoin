// Copyright David Abrahams, Daniel Wallin 2003.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This file generates overloads in this format:
//
//     template <typename A0, typename A1>
//     typename ::boost::mpl::apply_wrap1<
//         ::boost::parameter::aux::make_arg_list<
//             PS0,A0
//           , ::boost::parameter::aux::make_arg_list<
//                 PS1,A1
//               , ::boost::mpl::identity<
//                     ::boost::parameter::aux::empty_arg_list
//                 >
//             >
//         >
//      , unnamed_list
//     >::type
//         operator()(A0 const& a0, A1 const& a1) const
//     {
//         typedef typename ::boost::mpl::apply_wrap1<
//             ::boost::parameter::aux::make_arg_list<
//                 PS0,A0
//               , ::boost::parameter::aux::make_arg_list<
//                     PS1,A1
//                   , ::boost::mpl::identity<
//                         ::boost::parameter::aux::empty_arg_list
//                     >
//                 >
//             >
//         >::type arg_tuple;
//
//         return arg_tuple(
//             a0
//           , a1
//           , ::boost::parameter::aux::void_()
//             ...
//         );
//     }
//

#if !defined(BOOST_PP_IS_ITERATING)
# error Boost.Parameters - do not include this file!
#endif

#define N BOOST_PP_ITERATION()

#define BOOST_PARAMETER_open_list(z, n, text) \
    ::boost::parameter::aux::item< \
        BOOST_PP_CAT(PS, n), BOOST_PP_CAT(A, n)

#define BOOST_PARAMETER_close_list(z, n, text) > 

#define BOOST_PARAMETER_arg_list(n) \
    ::boost::parameter::aux::make_arg_list< \
        BOOST_PP_ENUM(N, BOOST_PARAMETER_open_list, _) \
      , ::boost::parameter::void_ \
        BOOST_PP_REPEAT(N, BOOST_PARAMETER_close_list, _) \
      , deduced_list \
      , ::boost::parameter::aux::tag_keyword_arg \
    >

#define BOOST_PARAMETER_arg_pack_init(z, n, limit) \
    BOOST_PP_CAT(a, BOOST_PP_SUB(limit, n))

template <BOOST_PP_ENUM_PARAMS(N, typename A)>
typename ::boost::mpl::first<
    typename BOOST_PARAMETER_arg_list(N)::type
>::type
    operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, A, & a)) const
{
    typedef typename BOOST_PARAMETER_arg_list(N)::type result;
    typedef typename ::boost::mpl::first<result>::type result_type;
    typedef typename ::boost::mpl::second<result>::type error;
    error();

    return result_type(
        BOOST_PP_ENUM(N, BOOST_PARAMETER_arg_pack_init, BOOST_PP_DEC(N))
        BOOST_PP_ENUM_TRAILING_PARAMS(
            BOOST_PP_SUB(BOOST_PARAMETER_COMPOSE_MAX_ARITY, N)
          , ::boost::parameter::aux::void_reference() BOOST_PP_INTERCEPT
        )
    );
}

#undef BOOST_PARAMETER_arg_list
#undef BOOST_PARAMETER_close_list
#undef BOOST_PARAMETER_open_list
#undef N

