// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_NORMALIZE_DEDUCED_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_NORMALIZE_DEDUCED_HPP_INCLUDED

#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/type_erasure/detail/meta.hpp>

#ifndef BOOST_TYPE_ERASURE_USE_MP11

namespace boost {
namespace type_erasure {
namespace detail {

template<class M, class T>
struct normalize_deduced;
template<class M, class T>
struct normalize_placeholder;

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/normalize_deduced.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

}
}
}

#endif

#endif

#else

#define N BOOST_PP_ITERATION()

#define BOOST_TYPE_ERASURE_NORMALIZE_PLACEHOLDER(z, n, data)        \
    typename ::boost::type_erasure::detail::normalize_placeholder<  \
        M,                                                          \
        BOOST_PP_CAT(U, n)                                          \
    >::type

template<class M, template<BOOST_PP_ENUM_PARAMS(N, class T)> class T, BOOST_PP_ENUM_PARAMS(N, class U)>
struct normalize_deduced<M, T<BOOST_PP_ENUM_PARAMS(N, U)> >
{
    typedef typename ::boost::type_erasure::deduced<
        T<BOOST_PP_ENUM(N, BOOST_TYPE_ERASURE_NORMALIZE_PLACEHOLDER, ~)>
    >::type type;
};

#undef BOOST_TYPE_ERASURE_NORMALIZE_PLACEHOLDER

#undef N

#endif
