// Copyright (C) 2005 Arkadiy Vertleyb
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_TYPE_TEMPLATE_PARAM_HPP_INCLUDED
#define BOOST_TYPEOF_TYPE_TEMPLATE_PARAM_HPP_INCLUDED

#define BOOST_TYPEOF_class_BOOST_TYPEOF (class)
#define BOOST_TYPEOF_typename_BOOST_TYPEOF (typename)

#define BOOST_TYPEOF_MAKE_OBJ_class BOOST_TYPEOF_TYPE_PARAM
#define BOOST_TYPEOF_MAKE_OBJ_typename BOOST_TYPEOF_TYPE_PARAM

#define BOOST_TYPEOF_TYPE_PARAM\
    (TYPE_PARAM)

#define BOOST_TYPEOF_TYPE_PARAM_EXPANDTYPE(Param) class

// TYPE_PARAM "virtual functions" implementation

#define BOOST_TYPEOF_TYPE_PARAM_ENCODE(This, n)\
    typedef typename boost::type_of::encode_type<\
        BOOST_PP_CAT(V, n),\
        BOOST_PP_CAT(P, n)\
    >::type BOOST_PP_CAT(V, BOOST_PP_INC(n)); 

#define BOOST_TYPEOF_TYPE_PARAM_DECODE(This, n)\
    typedef boost::type_of::decode_type< BOOST_PP_CAT(iter, n) > BOOST_PP_CAT(d, n);\
    typedef typename BOOST_PP_CAT(d, n)::type BOOST_PP_CAT(P, n);\
    typedef typename BOOST_PP_CAT(d, n)::iter BOOST_PP_CAT(iter, BOOST_PP_INC(n));

#define BOOST_TYPEOF_TYPE_PARAM_PLACEHOLDER(Param) int
#define BOOST_TYPEOF_TYPE_PARAM_DECLARATION_TYPE(Param) class
#define BOOST_TYPEOF_TYPE_PARAM_PLACEHOLDER_TYPES(Param, n) BOOST_PP_CAT(T,n)
#define BOOST_TYPEOF_TYPE_PARAM_ISTEMPLATE 0

#endif//BOOST_TYPEOF_TYPE_TEMPLATE_PARAM_HPP_INCLUDED
