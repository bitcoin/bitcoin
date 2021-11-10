// Copyright (C) 2004 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include <boost/typeof/encode_decode_params.hpp>

// member functions

template<class V, class T, class R BOOST_PP_ENUM_TRAILING_PARAMS(n, class P)> 
struct encode_type_impl<V, R(T::*)(BOOST_PP_ENUM_PARAMS(n, P)) BOOST_TYPEOF_qualifier>
{
    typedef R BOOST_PP_CAT(P, n);
    typedef T BOOST_PP_CAT(P, BOOST_PP_INC(n));
    typedef BOOST_TYPEOF_ENCODE_PARAMS(BOOST_PP_ADD(n, 2), BOOST_TYPEOF_id + n) type;
};

template<class Iter>
struct decode_type_impl<boost::type_of::constant<std::size_t,BOOST_TYPEOF_id + n>, Iter>
{
    typedef Iter iter0;
    BOOST_TYPEOF_DECODE_PARAMS(BOOST_PP_ADD(n, 2))
    template<class T> struct workaround{
        typedef BOOST_PP_CAT(p, n)(T::*type)(BOOST_PP_ENUM_PARAMS(n, p)) BOOST_TYPEOF_qualifier;
    };
    typedef typename workaround<BOOST_PP_CAT(p, BOOST_PP_INC(n))>::type type;
    typedef BOOST_PP_CAT(iter, BOOST_PP_ADD(n, 2)) iter;
};

// undef parameters

#undef BOOST_TYPEOF_id
#undef BOOST_TYPEOF_qualifier
